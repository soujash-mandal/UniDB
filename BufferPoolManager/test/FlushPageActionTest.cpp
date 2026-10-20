#include "../actions/FlushPageAction.h"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>

class FlushTestWritePagePort : public WritePagePort {
public:
  int writeCount = 0;
  BufferPoolPageId lastPageId = 0;
  char lastFirstByte = 0;
  bool shouldThrow = false;

  void writePage(BufferPoolPageId pageId, BufferPoolPage page) override {
    ++writeCount;
    lastPageId = pageId;
    lastFirstByte = page.data()[0];
    if (shouldThrow) {
      throw std::runtime_error("simulated disk write failure");
    }
  }
};

namespace {
void OccupyFlushFrame(BufferPool &pool, BufferPoolPageId pageId, bool dirty,
                      char firstByte = '\0') {
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

// Scenario 1: Flushing a dirty resident page writes the correct page and
// clears dirty state after success.
TEST(FlushPageActionTest, FlushesDirtyPageAndClearsDirtyFlag) {
  BufferPool pool(1);
  FlushTestWritePagePort writePort;
  OccupyFlushFrame(pool, 12, true, 'D');
  FlushPageAction action(pool, writePort);

  EXPECT_NO_THROW(action.execute(12));

  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(writePort.lastPageId, 12);
  EXPECT_EQ(writePort.lastFirstByte, 'D');
  EXPECT_FALSE(pool.getFrame(0).isDirty());
}

// Scenario 2: Explicit flush writes a clean resident page too, so callers can
// request persistence unconditionally.
TEST(FlushPageActionTest, FlushesCleanPageWhenExplicitlyRequested) {
  BufferPool pool(1);
  FlushTestWritePagePort writePort;
  OccupyFlushFrame(pool, 12, false, 'C');
  FlushPageAction action(pool, writePort);

  action.execute(12);

  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(writePort.lastPageId, 12);
  EXPECT_FALSE(pool.getFrame(0).isDirty());
}

// Scenario 3: If disk write fails, the page remains dirty so a later flush
// can retry.
TEST(FlushPageActionTest, PreservesDirtyFlagWhenWriteFails) {
  BufferPool pool(1);
  FlushTestWritePagePort writePort;
  writePort.shouldThrow = true;
  OccupyFlushFrame(pool, 12, true, 'F');
  FlushPageAction action(pool, writePort);

  EXPECT_THROW(action.execute(12), std::runtime_error);

  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_TRUE(pool.getFrame(0).isDirty());
  EXPECT_EQ(pool.getFrame(0).getPageId(), 12);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}

// Scenario 4: Flushing a page that is not resident reports a clear error and
// performs no I/O.
TEST(FlushPageActionTest, UnknownPageThrowsWithoutWriting) {
  BufferPool pool(1);
  FlushTestWritePagePort writePort;
  OccupyFlushFrame(pool, 12, true, 'X');
  FlushPageAction action(pool, writePort);

  EXPECT_THROW(action.execute(99), std::runtime_error);

  EXPECT_EQ(writePort.writeCount, 0);
  EXPECT_TRUE(pool.getFrame(0).isDirty());
  EXPECT_EQ(pool.getFrame(0).getPageId(), 12);
}

// Scenario 5: A transient write failure can be retried, and only the
// successful retry clears the dirty flag.
TEST(FlushPageActionTest, SuccessfulRetryClearsDirtyFlag) {
  BufferPool pool(1);
  FlushTestWritePagePort writePort;
  writePort.shouldThrow = true;
  OccupyFlushFrame(pool, 12, true, 'R');
  FlushPageAction action(pool, writePort);

  EXPECT_THROW(action.execute(12), std::runtime_error);
  EXPECT_TRUE(pool.getFrame(0).isDirty());

  writePort.shouldThrow = false;
  EXPECT_NO_THROW(action.execute(12));
  EXPECT_EQ(writePort.writeCount, 2);
  EXPECT_EQ(writePort.lastPageId, 12);
  EXPECT_FALSE(pool.getFrame(0).isDirty());
}
