#include "../adapter/FileDiskManagerAdapter.h"

#include <gtest/gtest.h>

#include <cstdio>

TEST(DiskManagerTest, WritesAndReadsPage) {
  const char *testFile = "test_database.db";

  {
    FileDiskManagerAdapter disk(testFile);

    DiskPage page;

    page.data()[0] = 'U';
    page.data()[1] = 'D';
    page.data()[2] = 'B';

    disk.writePage(42, page);
  }

  {
    FileDiskManagerAdapter disk(testFile);

    DiskPage page = disk.readPage(42);

    EXPECT_EQ(page.data()[0], 'U');
    EXPECT_EQ(page.data()[1], 'D');
    EXPECT_EQ(page.data()[2], 'B');
  }

  std::remove(testFile);
}
