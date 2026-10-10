#include "../adapter/FileDiskManagerAdapter.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

TEST(DiskManagerTest, WritesAndReadsPage) {
  const char *testFile = "test_database.db";
  std::remove(testFile);

  {
    std::ofstream file(testFile, std::ios::binary);
    ASSERT_TRUE(file.is_open());
  }

  {
    FileDiskManagerAdapter disk;
    disk.setFileName(testFile);

    DiskPage page;
    page.data()[0] = 'U';
    page.data()[1] = 'D';
    page.data()[2] = 'B';

    disk.writePage(42, page);
  }

  {
    FileDiskManagerAdapter disk;
    disk.setFileName(testFile);

    DiskPage page = disk.readDiskPage(42);

    EXPECT_EQ(page.data()[0], 'U');
    EXPECT_EQ(page.data()[1], 'D');
    EXPECT_EQ(page.data()[2], 'B');
  }

  std::remove(testFile);
}

TEST(DiskManagerTest, DoesNotCreateMissingDatabaseFile) {
  const char *missingFile = "missing_test_database.db";
  std::remove(missingFile);

  FileDiskManagerAdapter disk;
  disk.setFileName(missingFile);

  EXPECT_THROW(disk.readDiskPage(0), std::runtime_error);
  EXPECT_FALSE(std::ifstream(missingFile).good());
}
