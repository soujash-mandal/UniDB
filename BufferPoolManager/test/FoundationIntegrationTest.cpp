#include "../../DatabaseStartup.h"

#include <gtest/gtest.h>

#include <filesystem>
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
