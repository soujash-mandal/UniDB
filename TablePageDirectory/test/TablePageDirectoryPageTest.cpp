#include "../domain/TablePageDirectoryPage.h"

#include <gtest/gtest.h>

#include <cstring>
#include <stdexcept>

TEST(TablePageDirectoryPageTest, StartsEmptyWithNoNextPage) {
  TablePageDirectoryPage page;

  EXPECT_EQ(page.size(), 0U);
  EXPECT_FALSE(page.isFull());
  EXPECT_EQ(page.getNextPageId(), TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID);
  EXPECT_EQ(page.findPageWithSpace(1),
            TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID);
}

TEST(TablePageDirectoryPageTest, InsertsAndLooksUpTuplePageFreeSpace) {
  TablePageDirectoryPage page;
  page.insert(12, 400);
  page.insert(18, 100);

  EXPECT_EQ(page.size(), 2U);
  EXPECT_TRUE(page.contains(12));
  EXPECT_TRUE(page.contains(18));
  EXPECT_FALSE(page.contains(99));
  EXPECT_EQ(page.getFreeSpace(12), 400U);
  EXPECT_EQ(page.getFreeSpace(18), 100U);
}

TEST(TablePageDirectoryPageTest, FindsFirstPageWithEnoughFreeSpace) {
  TablePageDirectoryPage page;
  page.insert(12, 100);
  page.insert(18, 400);

  EXPECT_EQ(page.findPageWithSpace(300), 18U);
  EXPECT_EQ(page.findPageWithSpace(401),
            TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID);
  EXPECT_EQ(page.findPageWithSpace(0), 12U);
}

TEST(TablePageDirectoryPageTest, UpdatesExistingEntryWithoutAddingAnother) {
  TablePageDirectoryPage page;
  page.insert(12, 100);

  page.update(12, 500);

  EXPECT_EQ(page.size(), 1U);
  EXPECT_EQ(page.getFreeSpace(12), 500U);
  EXPECT_EQ(page.findPageWithSpace(400), 12U);
}

TEST(TablePageDirectoryPageTest, RejectsDuplicateAndInvalidEntries) {
  TablePageDirectoryPage page;
  page.insert(12, 100);

  EXPECT_THROW(page.insert(12, 200), std::runtime_error);
  EXPECT_THROW(page.insert(TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID, 100),
               std::runtime_error);
  EXPECT_THROW(page.insert(13, TablePageDirectoryPage::PAGE_SIZE + 1),
               std::runtime_error);
  EXPECT_EQ(page.size(), 1U);
  EXPECT_EQ(page.getFreeSpace(12), 100U);
}

TEST(TablePageDirectoryPageTest, MissingEntriesFailClearly) {
  TablePageDirectoryPage page;

  EXPECT_THROW(page.getFreeSpace(12), std::runtime_error);
  EXPECT_THROW(page.update(12, 100), std::runtime_error);
}

TEST(TablePageDirectoryPageTest, RejectsFreeSpaceLargerThanPageSizeOnUpdate) {
  TablePageDirectoryPage page;
  page.insert(12, 100);

  EXPECT_THROW(page.update(12, TablePageDirectoryPage::PAGE_SIZE + 1),
               std::runtime_error);
  EXPECT_EQ(page.getFreeSpace(12), 100U);
}

TEST(TablePageDirectoryPageTest, NextPageIdAndSerializedBytesArePreserved) {
  TablePageDirectoryPage original;
  original.insert(12, 1234);
  original.setNextPageId(42);

  TablePageDirectoryPage restored;
  std::memcpy(restored.data(), original.data(),
              TablePageDirectoryPage::PAGE_SIZE);

  EXPECT_EQ(restored.getNextPageId(), 42U);
  EXPECT_EQ(restored.size(), 1U);
  EXPECT_EQ(restored.getFreeSpace(12), 1234U);
}
