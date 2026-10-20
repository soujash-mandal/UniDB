#include "../actions/WritePageAction.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {
// Scenario helper: install a known resident page for WritePageAction tests.
void OccupyWriteFrame(BufferPool &pool, BufferPoolPageId pageId, char firstByte,
                      bool dirty) {
  Frame &frame = pool.getFrame(0);
  BufferPoolPage page;
  page.data()[0] = firstByte;
  frame.setPage(page);
  frame.setPageId(pageId);
  frame.setOccupied(true);
  frame.setDirty(dirty);
  frame.pin();
}
} // namespace

// Scenario 1: Writing a page replaces its full byte contents and marks it
// dirty.
TEST(WritePageActionTest, WritesAllBytesAndMarksPageDirty) {
  BufferPool pool(1);
  OccupyWriteFrame(pool, 12, 'A', false);
  BufferPoolPage replacement;
  for (std::size_t i = 0; i < BufferPoolPage::PAGE_SIZE; ++i) {
    replacement.data()[i] = static_cast<char>(i % 251);
  }
  WritePageAction action(pool);

  action.execute(12, replacement);

  const BufferPoolPage actual = pool.getFrame(0).getPage();
  for (std::size_t i = 0; i < BufferPoolPage::PAGE_SIZE; ++i) {
    EXPECT_EQ(actual.data()[i], static_cast<char>(i % 251));
  }
  EXPECT_TRUE(pool.getFrame(0).isDirty());
}

// Scenario 2: A write changes only the requested resident page.
TEST(WritePageActionTest, LeavesOtherResidentPagesUnchanged) {
  BufferPool pool(2);
  OccupyWriteFrame(pool, 12, 'A', false);
  Frame &other = pool.getFrame(1);
  BufferPoolPage otherPage;
  otherPage.data()[0] = 'B';
  other.setPage(otherPage);
  other.setPageId(13);
  other.setOccupied(true);
  other.setDirty(false);
  other.pin();

  BufferPoolPage replacement;
  replacement.data()[0] = 'Z';
  WritePageAction action(pool);
  action.execute(12, replacement);

  EXPECT_EQ(pool.getFrame(0).getPage().data()[0], 'Z');
  EXPECT_TRUE(pool.getFrame(0).isDirty());
  EXPECT_EQ(other.getPage().data()[0], 'B');
  EXPECT_FALSE(other.isDirty());
}

// Scenario 3: An unknown PageId fails without modifying any resident page.
TEST(WritePageActionTest, UnknownPageThrowsWithoutChangingResidentPage) {
  BufferPool pool(1);
  OccupyWriteFrame(pool, 12, 'A', false);
  WritePageAction action(pool);
  BufferPoolPage replacement;
  replacement.data()[0] = 'Z';

  EXPECT_THROW(action.execute(99, replacement), std::runtime_error);

  EXPECT_EQ(pool.getFrame(0).getPageId(), 12);
  EXPECT_EQ(pool.getFrame(0).getPage().data()[0], 'A');
  EXPECT_FALSE(pool.getFrame(0).isDirty());
}

// Scenario 4: Binary boundary bytes are preserved exactly by a page write.
TEST(WritePageActionTest, PreservesBinaryBoundaryBytes) {
  BufferPool pool(1);
  OccupyWriteFrame(pool, 12, 'A', false);
  BufferPoolPage replacement;
  replacement.data()[0] = '\0';
  replacement.data()[BufferPoolPage::PAGE_SIZE - 1] = static_cast<char>(0xFF);
  WritePageAction action(pool);

  action.execute(12, replacement);

  const BufferPoolPage actual = pool.getFrame(0).getPage();
  EXPECT_EQ(actual.data()[0], '\0');
  EXPECT_EQ(
      static_cast<unsigned char>(actual.data()[BufferPoolPage::PAGE_SIZE - 1]),
      0xFF);
  EXPECT_TRUE(pool.getFrame(0).isDirty());
}
