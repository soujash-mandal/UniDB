#include "../domain/FSMPage.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(FSMPageTest, InitializesWithNoNextPageAndNoEntries) {
  FSMPage page;
  EXPECT_EQ(page.getNextPageId(), INVALID_PAGE_ID);
  EXPECT_EQ(page.findPageWithSpace(1), INVALID_PAGE_ID);
  EXPECT_FALSE(page.isFull());
}

TEST(FSMPageTest, InsertsFindsAndUpdatesPageFreeSpace) {
  FSMPage page;
  page.insert(42, 512);
  page.insert(43, 128);

  EXPECT_EQ(page.findPageWithSpace(400), 42);
  EXPECT_EQ(page.findPageWithSpace(129), 42);
  EXPECT_EQ(page.findPageWithSpace(513), INVALID_PAGE_ID);

  page.update(42, 64);
  EXPECT_EQ(page.findPageWithSpace(100), 43);
  EXPECT_THROW(page.update(99, 100), std::runtime_error);
}

TEST(FSMPageTest, StoresNextPageAndSupportsFullCapacity) {
  FSMPage page;
  page.setNextPageId(7);
  EXPECT_EQ(page.getNextPageId(), 7);

  for (uint32_t i = 0; i < MAX_ENTRIES; ++i) {
    page.insert(i + 1, i);
  }
  EXPECT_TRUE(page.isFull());
  EXPECT_THROW(page.insert(MAX_ENTRIES + 1, 1), std::runtime_error);
}
