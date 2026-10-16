#include "../../DatabaseStartup.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {
std::filesystem::path IntegrationDatabasePath() {
  return std::filesystem::temp_directory_path() /
         "unidb_foundation_reopen_integration_test.db";
}
} // namespace

// Scenario 1: A fresh database initializes once; allocated page bytes and
// allocation metadata survive Container destruction/reopen.
TEST(FoundationIntegrationTest,
     FreshStartupPersistsPagesAndAllocationMetadataAcrossReopen) {
  const auto path = IntegrationDatabasePath();
  std::filesystem::remove(path);

  uint32_t dataPageId = 0;
  {
    Container container(path.string(), 2, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    dataPageId = container.newPageAction().execute();
    BufferPoolPage expected;
    for (std::size_t i = 0; i < BufferPoolPage::PAGE_SIZE; ++i) {
      expected.data()[i] = static_cast<char>((i * 19U) % 256U);
    }

    container.writePageAction().execute(dataPageId, expected);
    container.unpinPageAction().execute(dataPageId);
    container.flushPageAction().execute(dataPageId);
  }

  {
    Container reopened(path.string(), 2, EvictionPolicyType::FIFO);
    EXPECT_FALSE(DatabaseStartup::execute(reopened));

    const BufferPoolPage actual =
        reopened.fetchPageAction().execute(dataPageId);
    for (std::size_t i = 0; i < BufferPoolPage::PAGE_SIZE; ++i) {
      EXPECT_EQ(static_cast<unsigned char>(actual.data()[i]),
                static_cast<unsigned char>((i * 19U) % 256U))
          << "Persisted byte mismatch at offset " << i;
    }
    reopened.unpinPageAction().execute(dataPageId);

    const uint32_t nextPageId = reopened.newPageAction().execute();
    EXPECT_NE(nextPageId, dataPageId);
    EXPECT_NE(nextPageId, 0);
    EXPECT_NE(nextPageId, 1);
    EXPECT_NE(nextPageId, 2);
    reopened.unpinPageAction().execute(nextPageId);
    reopened.flushPageAction().execute(nextPageId);
  }

  std::filesystem::remove(path);
}

// Scenario 2: A full first FPM bitmap is persisted to a real database file;
// the next allocation creates a chained metadata page, and that chain remains
// usable after the Container is destroyed and reopened.
TEST(FoundationIntegrationTest,
     FpmMetadataChainPersistsAndAllocatesAfterReopen) {
  const auto path = std::filesystem::temp_directory_path() /
                    "unidb_fpm_metadata_chain_reopen_test.db";
  std::filesystem::remove(path);

  {
    Container container(path.string(), 2, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
  }

  // Fill the root bitmap directly so the test exercises the chain boundary
  // without performing tens of thousands of individual allocations.
  MetadataPage fullRoot;
  for (uint32_t pageId = 1; pageId < MAX_TRACKED_PAGES; ++pageId) {
    fullRoot.setPageOccupied(pageId);
  }

  FileDiskManagerAdapter disk;
  disk.fileName = path.string();
  WriteDiskPageAction writeDiskPage(disk);
  WriteFreePageMetadataAdapter writeMetadata(writeDiskPage);
  writeMetadata.writePage(0, fullRoot);

  const uint32_t expectedFirstDataPage = MAX_TRACKED_PAGES + 1;
  {
    Container reopened(path.string(), 2, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    const uint32_t firstDataPage = reopened.newPageAction().execute();
    ASSERT_EQ(firstDataPage, expectedFirstDataPage);
    reopened.unpinPageAction().execute(firstDataPage);
    reopened.flushPageAction().execute(firstDataPage);
  }

  {
    Container reopenedAgain(path.string(), 2, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopenedAgain));

    const uint32_t nextDataPage = reopenedAgain.newPageAction().execute();
    EXPECT_EQ(nextDataPage, expectedFirstDataPage + 1);
    EXPECT_NE(nextDataPage, MAX_TRACKED_PAGES);
    reopenedAgain.unpinPageAction().execute(nextDataPage);
    reopenedAgain.flushPageAction().execute(nextDataPage);
  }

  std::filesystem::remove(path);
}

// Scenario 3: Startup rejects a truncated pre-existing database and leaves
// the file untouched instead of treating it as a new database.
TEST(FoundationIntegrationTest,
     TruncatedExistingDatabaseIsRejectedWithoutReinitialization) {
  const auto path = std::filesystem::temp_directory_path() /
                    "unidb_truncated_existing_database_test.db";
  std::filesystem::remove(path);
  {
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file.put('X');
    ASSERT_TRUE(file.good());
  }

  {
    Container container(path.string(), 2, EvictionPolicyType::FIFO);
    EXPECT_THROW(DatabaseStartup::execute(container), std::runtime_error);
  }

  ASSERT_TRUE(std::filesystem::exists(path));
  EXPECT_EQ(std::filesystem::file_size(path), 1);
  std::filesystem::remove(path);
}
