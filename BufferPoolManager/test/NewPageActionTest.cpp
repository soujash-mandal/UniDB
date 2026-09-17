#include <gtest/gtest.h>

#include "../actions/NewPageAction.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"

class FakeAllocatePagePort : public AllocatePagePort {

public:
  PageId nextPageId = 0;

  PageId allocatePage() override { return nextPageId++; }
};

// 1. New page should be placed into an empty frame
TEST(NewPageActionTest, CreatesPageInEmptyFrame) {

  BufferPool bufferPool(2);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  Page &page = newPage.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 0);
  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// 2. New page should return the page stored in the frame
TEST(NewPageActionTest, ReturnsPageFromBufferPool) {

  BufferPool bufferPool(2);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  Page &page = newPage.execute();

  page.data()[0] = 'X';

  EXPECT_EQ(bufferPool.getFrame(0).getPage().data()[0], 'X');
}

// 3. Each new page should receive a different page ID
TEST(NewPageActionTest, AllocatesDifferentPageIds) {

  BufferPool bufferPool(2);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();
  newPage.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 0);
  EXPECT_EQ(bufferPool.getFrame(1).getPageId(), 1);
}

// 4. New pages should occupy different frames
TEST(NewPageActionTest, CreatesPagesInDifferentFrames) {

  BufferPool bufferPool(2);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();
  newPage.execute();

  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_TRUE(bufferPool.getFrame(1).isOccupied());

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 0);
  EXPECT_EQ(bufferPool.getFrame(1).getPageId(), 1);
}

// 5. New page should start with pin count 1
TEST(NewPageActionTest, NewPageStartsPinned) {

  BufferPool bufferPool(1);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}

// 6. New page should start as dirty
TEST(NewPageActionTest, NewPageStartsDirty) {

  BufferPool bufferPool(1);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// 7. New page should be marked occupied
TEST(NewPageActionTest, NewPageIsOccupied) {

  BufferPool bufferPool(1);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();

  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
}

// 8. New page should use the first available frame
TEST(NewPageActionTest, UsesFirstAvailableFrame) {

  BufferPool bufferPool(3);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();
  newPage.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 0);
  EXPECT_EQ(bufferPool.getFrame(1).getPageId(), 1);

  EXPECT_FALSE(bufferPool.getFrame(2).isOccupied());
}

// 9. Creating a new page when buffer pool is full should throw
TEST(NewPageActionTest, ThrowsWhenBufferPoolIsFull) {

  BufferPool bufferPool(2);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();
  newPage.execute();

  EXPECT_THROW(newPage.execute(), std::runtime_error);
}

// 10. NewPageAction should request a new page ID every time
TEST(NewPageActionTest, RequestsNewPageIdEveryTime) {

  BufferPool bufferPool(3);
  FakeAllocatePagePort allocatePagePort;

  NewPageAction newPage(bufferPool, allocatePagePort);

  newPage.execute();
  newPage.execute();
  newPage.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 0);

  EXPECT_EQ(bufferPool.getFrame(1).getPageId(), 1);

  EXPECT_EQ(bufferPool.getFrame(2).getPageId(), 2);
}
