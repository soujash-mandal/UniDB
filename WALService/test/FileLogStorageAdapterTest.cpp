#include "../adapter/FileLogStorageAdapter.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <vector>

namespace {
std::filesystem::path TestFile(const char *name) {
  return std::filesystem::temp_directory_path() / name;
}
} // namespace

TEST(FileLogStorageAdapterTest, WritesAndReadsBinaryBytesAtAnOffset) {
  const auto path = TestFile("unidb_wal_storage_binary_test.log");
  std::filesystem::remove(path);

  {
    FileLogStorageAdapter storage(path.string());
    const std::vector<std::byte> expected = {
        std::byte{0x00}, std::byte{0x7f}, std::byte{0x80}, std::byte{0xff}};
    storage.writeAt(11, expected);
    storage.flush();

    EXPECT_EQ(storage.readAt(11, expected.size()), expected);
  }

  std::filesystem::remove(path);
}

TEST(FileLogStorageAdapterTest, KeepsWritesAtDifferentOffsetsIndependent) {
  const auto path = TestFile("unidb_wal_storage_offsets_test.log");
  std::filesystem::remove(path);

  {
    FileLogStorageAdapter storage(path.string());
    const std::vector<std::byte> first = {std::byte{'A'}, std::byte{'B'}};
    const std::vector<std::byte> second = {std::byte{'X'}};

    storage.writeAt(0, first);
    storage.writeAt(8, second);
    storage.flush();

    EXPECT_EQ(storage.readAt(0, 2), first);
    EXPECT_EQ(storage.readAt(8, 1), second);
  }

  std::filesystem::remove(path);
}

TEST(FileLogStorageAdapterTest, RejectsReadsBeyondTheAvailableFileBytes) {
  const auto path = TestFile("unidb_wal_storage_short_read_test.log");
  std::filesystem::remove(path);

  {
    FileLogStorageAdapter storage(path.string());
    const std::vector<std::byte> bytes = {std::byte{0x01}};
    storage.writeAt(0, bytes);
    storage.flush();

    EXPECT_THROW(storage.readAt(0, 2), std::runtime_error);
  }

  std::filesystem::remove(path);
}

TEST(FileLogStorageAdapterTest, EmptyReadAndWriteAreNoOps) {
  const auto path = TestFile("unidb_wal_storage_empty_test.log");
  std::filesystem::remove(path);

  {
    FileLogStorageAdapter storage(path.string());
    const std::vector<std::byte> empty;

    EXPECT_NO_THROW(storage.writeAt(0, empty));
    EXPECT_TRUE(storage.readAt(0, 0).empty());
  }

  std::filesystem::remove(path);
}
