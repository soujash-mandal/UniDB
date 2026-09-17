#include <gtest/gtest.h>

#include "../actions/FetchPageAction.h"
#include "../actions/UnpinPageAction.h"
#include "../domain/BufferPool.h"
#include "../port/ReadPagePort.h"

class FakeReadPagePort : public ReadPagePort {

public:
  void readPage(PageId pageId, Page &page) override {
    page.data()[0] = 'A' + pageId;
  }
};

// 1. Unpin a fetched page
TEST(UnpinPageActionTest, UnpinsPage) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
}

// 2. Unpinning with dirty = true marks the page dirty
TEST(UnpinPageActionTest, MarksPageDirtyWhenDirtyIsTrue) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());

  unpinPage.execute(5, true);

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// 3. Unpinning with dirty = false does not mark a clean page dirty
TEST(UnpinPageActionTest, DoesNotMarkPageDirtyWhenDirtyIsFalse) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());

  unpinPage.execute(5, false);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
}

// 4. Multiple pins require multiple unpins
TEST(UnpinPageActionTest, MultipleUnpinsDecreasePinCount) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);
  fetchPage.execute(5);
  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 3);

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 2);

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
}

// 5. Pin count should not go below zero
TEST(UnpinPageActionTest, PinCountDoesNotGoBelowZero) {

  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);

  // Unpin again
  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
}

// 6. Unpinning one page should not affect another page
TEST(UnpinPageActionTest, DoesNotAffectOtherPages) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);
  fetchPage.execute(10);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(bufferPool.getFrame(1).getPinCount(), 1);

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(bufferPool.getFrame(1).getPinCount(), 1);
}

// 7. Marking a page dirty should persist across later unpins
TEST(UnpinPageActionTest, DirtyFlagRemainsTrueAfterLaterUnpin) {

  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);
  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 2);

  unpinPage.execute(5, true);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());

  unpinPage.execute(5, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// 8. Unpinning a page that is not in the buffer pool should throw
TEST(UnpinPageActionTest, ThrowsWhenPageIsNotFound) {

  BufferPool bufferPool(2);

  UnpinPageAction unpinPage(bufferPool);

  EXPECT_THROW(unpinPage.execute(5, false), std::runtime_error);
}

// 9. Unpinning another page should throw
TEST(UnpinPageActionTest, ThrowsWhenRequestedPageIsNotInBufferPool) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);

  EXPECT_THROW(unpinPage.execute(10, false), std::runtime_error);

  // Original page remains unchanged
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 5);
}

// 10. Dirty flag can be set even when pin count reaches zero
TEST(UnpinPageActionTest, DirtyPageCanBecomeUnpinned) {

  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);
  UnpinPageAction unpinPage(bufferPool);

  fetchPage.execute(5);

  unpinPage.execute(5, true);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}
