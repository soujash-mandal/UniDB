#include <gtest/gtest.h>

#include "../actions/FetchPageAction.h"
#include "../domain/BufferPool.h"
#include "../port/ReadPagePort.h"

class FakeReadPagePort : public ReadPagePort {

public:
  void readPage(PageId pageId, Page &page) override {
    page.data()[0] = 'A' + pageId;
  }
};

// 1. Fetch a page into an empty frame
TEST(FetchPageActionTest, FetchesPageIntoEmptyFrame) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  Page &page = fetchPage.execute(5);

  EXPECT_EQ(page.data()[0], 'F');
  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 5);
  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
}

// 2. Fetching the same page should return the existing page
TEST(FetchPageActionTest, FetchExistingPageReturnsSamePage) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  Page &firstPage = fetchPage.execute(5);

  firstPage.data()[0] = 'X';

  Page &secondPage = fetchPage.execute(5);

  EXPECT_EQ(&firstPage, &secondPage);
  EXPECT_EQ(secondPage.data()[0], 'X');
}

// 3. Fetching the same page should increase pin count
TEST(FetchPageActionTest, FetchExistingPageIncreasesPinCount) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 2);

  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 3);
}

// 4. Different pages should use different empty frames
TEST(FetchPageActionTest, FetchesDifferentPagesIntoDifferentFrames) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(5);
  fetchPage.execute(10);

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 5);
  EXPECT_EQ(bufferPool.getFrame(1).getPageId(), 10);

  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_TRUE(bufferPool.getFrame(1).isOccupied());
}

// 5. Each fetched page should contain the data read from the port
TEST(FetchPageActionTest, FetchesCorrectPageData) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  Page &page1 = fetchPage.execute(1);
  Page &page2 = fetchPage.execute(2);

  EXPECT_EQ(page1.data()[0], 'B');
  EXPECT_EQ(page2.data()[0], 'C');
}

// 6. A fetched page should be marked occupied
TEST(FetchPageActionTest, FetchedPageIsOccupied) {

  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(5);

  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
}

// 7. A fetched page should start as clean
TEST(FetchPageActionTest, FetchedPageIsClean) {

  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(5);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
}

// 8. A fetched page should start with pin count 1
TEST(FetchPageActionTest, FetchedPageStartsPinned) {

  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(5);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}

// 9. Fetch should use the first available empty frame
TEST(FetchPageActionTest, UsesFirstEmptyFrame) {

  BufferPool bufferPool(3);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(10);
  fetchPage.execute(20);

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 10);
  EXPECT_EQ(bufferPool.getFrame(1).getPageId(), 20);

  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_TRUE(bufferPool.getFrame(1).isOccupied());
  EXPECT_FALSE(bufferPool.getFrame(2).isOccupied());
}

// 10. Fetch should fail when the buffer pool is full
TEST(FetchPageActionTest, ThrowsWhenBufferPoolIsFull) {

  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;

  FetchPageAction fetchPage(bufferPool, readPagePort);

  fetchPage.execute(5);
  fetchPage.execute(10);

  EXPECT_THROW(fetchPage.execute(15), std::runtime_error);
}
