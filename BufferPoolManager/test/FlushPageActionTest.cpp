#include <gtest/gtest.h>

#include "../../EvictionPolicy/FIFOEvictionPolicy.h"
#include "../actions/FetchPageAction.h"
#include "../actions/FlushPageAction.h"
#include "../actions/UnpinPageAction.h"
#include "../domain/BufferPool.h"
#include "../port/ReadPagePort.h"
#include "../port/WritePagePort.h"

class FakeReadPagePort : public ReadPagePort {

public:
  void readPage(PageId pageId, Page &page) override {
    page.data()[0] = 'A' + pageId;
  }
};

class FakeWritePagePort : public WritePagePort {

public:
  PageId lastPageId = 0;
  int writeCount = 0;
  char lastData = '\0';

  void writePage(PageId pageId, const Page &page) override {
    lastPageId = pageId;
    lastData = page.data()[0];
    writeCount++;
  }
};

// 1. Flush a page writes it to disk
TEST(FlushPageActionTest, FlushesPage) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  Page &page = fetchPage.execute(5);
  page.data()[0] = 'X';

  flushPage.execute(5);

  EXPECT_EQ(writePagePort.writeCount, 1);
  EXPECT_EQ(writePagePort.lastPageId, 5);
  EXPECT_EQ(writePagePort.lastData, 'X');
}

// 2. Flush should mark the page as clean
TEST(FlushPageActionTest, MarksPageCleanAfterFlush) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);
  UnpinPageAction unpinPage(bufferPool, evictionPolicy);

  Page &page = fetchPage.execute(5);
  page.data()[0] = 'X';

  unpinPage.execute(5, true);

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());

  flushPage.execute(5);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
}

// 3. Flush should write the correct page ID
TEST(FlushPageActionTest, WritesCorrectPageId) {
  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  fetchPage.execute(5);
  fetchPage.execute(10);

  flushPage.execute(10);

  EXPECT_EQ(writePagePort.lastPageId, 10);
}

// 4. Flush should write the current page data
TEST(FlushPageActionTest, WritesCurrentPageData) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  Page &page = fetchPage.execute(5);
  page.data()[0] = 'Z';

  flushPage.execute(5);

  EXPECT_EQ(writePagePort.lastData, 'Z');
}

// 5. Flushing a clean page should still write it
TEST(FlushPageActionTest, FlushesCleanPage) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  fetchPage.execute(5);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());

  flushPage.execute(5);

  EXPECT_EQ(writePagePort.writeCount, 1);
}

// 6. Flushing should only affect the requested page
TEST(FlushPageActionTest, DoesNotAffectOtherPages) {
  BufferPool bufferPool(2);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);
  UnpinPageAction unpinPage(bufferPool, evictionPolicy);

  fetchPage.execute(5);
  fetchPage.execute(10);

  unpinPage.execute(5, true);
  unpinPage.execute(10, true);

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
  EXPECT_TRUE(bufferPool.getFrame(1).isDirty());

  flushPage.execute(5);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
  EXPECT_TRUE(bufferPool.getFrame(1).isDirty());

  EXPECT_EQ(writePagePort.lastPageId, 5);
}

// 7. Flushing should work even when page is pinned
TEST(FlushPageActionTest, FlushesPinnedPage) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  Page &page = fetchPage.execute(5);
  page.data()[0] = 'Q';

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  flushPage.execute(5);

  EXPECT_EQ(writePagePort.writeCount, 1);
  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}

// 8. Flush should throw when page is not in the buffer pool
TEST(FlushPageActionTest, ThrowsWhenPageIsNotFound) {
  BufferPool bufferPool(1);
  FakeWritePagePort writePagePort;

  FlushPageAction flushPage(bufferPool, writePagePort);

  EXPECT_THROW(flushPage.execute(5), std::runtime_error);
  EXPECT_EQ(writePagePort.writeCount, 0);
}

// 9. Flushing the same page twice writes it twice
TEST(FlushPageActionTest, CanFlushSamePageMultipleTimes) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  Page &page = fetchPage.execute(5);

  page.data()[0] = 'X';

  flushPage.execute(5);

  EXPECT_EQ(writePagePort.writeCount, 1);
  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());

  page.data()[0] = 'Y';

  flushPage.execute(5);

  EXPECT_EQ(writePagePort.writeCount, 2);
  EXPECT_EQ(writePagePort.lastData, 'Y');
  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
}

// 10. Flush should preserve the page in the buffer pool
TEST(FlushPageActionTest, DoesNotRemovePageFromBufferPool) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FIFOEvictionPolicy evictionPolicy;

  FetchPageAction fetchPage(bufferPool, readPagePort, writePagePort,
                            evictionPolicy);
  FlushPageAction flushPage(bufferPool, writePagePort);

  Page &page = fetchPage.execute(5);
  page.data()[0] = 'X';

  flushPage.execute(5);

  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 5);
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}
