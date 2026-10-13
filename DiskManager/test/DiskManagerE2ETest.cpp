#include "../adapter/FileDiskManagerAdapter.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
std::filesystem::path TestFile(const char *name) {
  return std::filesystem::temp_directory_path() / name;
}

void CreateEmptyFile(const std::filesystem::path &path) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  ASSERT_TRUE(file.is_open());
}
} // namespace

// Scenario 1: A full 8192-byte page, including zero and high-bit binary bytes, survives write/read exactly.
TEST(DiskManagerTest, WritesAndReadsEveryByteOfPage) {
  const auto path = TestFile("unidb_disk_full_page_test.db");
  std::filesystem::remove(path);
  CreateEmptyFile(path);

  FileDiskManagerAdapter disk;
  disk.fileName = path.string();
  DiskPage expected;

  for (std::size_t i = 0; i < DiskPage::PAGE_SIZE; ++i) {
    expected.data()[i] = static_cast<char>((i * 37U) % 256U);
  }

  disk.writePage(42, expected);
  const DiskPage actual = disk.readDiskPage(42);

  for (std::size_t i = 0; i < DiskPage::PAGE_SIZE; ++i) {
    EXPECT_EQ(static_cast<unsigned char>(actual.data()[i]),
              static_cast<unsigned char>(expected.data()[i]))
        << "Byte mismatch at offset " << i;
  }

  std::filesystem::remove(path);
}

// Scenario 2: Writing and overwriting one page does not alter neighboring pages or use the wrong page offset.
TEST(DiskManagerTest, MultiplePagesRemainIsolatedWhenOnePageIsOverwritten) {
  const auto path = TestFile("unidb_disk_multiple_pages_test.db");
  std::filesystem::remove(path);
  CreateEmptyFile(path);

  FileDiskManagerAdapter disk;
  disk.fileName = path.string();
  DiskPage pageA;
  DiskPage pageB;
  DiskPage pageC;
  pageA.data()[0] = 'A';
  pageB.data()[0] = 'B';
  pageC.data()[0] = 'C';

  disk.writePage(0, pageA);
  disk.writePage(1, pageB);
  disk.writePage(2, pageC);

  DiskPage replacement;
  replacement.data()[0] = 'X';
  disk.writePage(1, replacement);

  EXPECT_EQ(disk.readDiskPage(0).data()[0], 'A');
  EXPECT_EQ(disk.readDiskPage(1).data()[0], 'X');
  EXPECT_EQ(disk.readDiskPage(2).data()[0], 'C');
  EXPECT_EQ(std::filesystem::file_size(path), 3 * DiskPage::PAGE_SIZE);

  std::filesystem::remove(path);
}

// Scenario 3: Reading a missing database file fails and does not create the file as a side effect.
TEST(DiskManagerTest, DoesNotCreateMissingDatabaseFile) {
  const auto path = TestFile("unidb_disk_missing_database_test.db");
  std::filesystem::remove(path);

  FileDiskManagerAdapter disk;
  disk.fileName = path.string();

  EXPECT_THROW(disk.readDiskPage(0), std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(path));
}

// Scenario 4: A file shorter than one page is rejected instead of returning partially initialized page bytes.
TEST(DiskManagerTest, RejectsShortPageRead) {
  const auto path = TestFile("unidb_disk_short_page_test.db");
  std::filesystem::remove(path);
  {
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    std::string bytes(DiskPage::PAGE_SIZE - 1, 'S');
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(file.good());
  }

  FileDiskManagerAdapter disk;
  disk.fileName = path.string();

  EXPECT_THROW(disk.readDiskPage(0), std::runtime_error);

  std::filesystem::remove(path);
}
