#include "../actions/NewPageAction.h"

#include <gtest/gtest.h>

#include <optional>

class NewPageTestAllocatePort : public AllocatePagePort {
public:
  BufferPoolPageId nextPageId = 2;
  BufferPoolPageId allocatePage() override { return nextPageId++; }
};

class NewPageTestWritePort : public WritePagePort {
public:
  int writeCount = 0;
  BufferPoolPageId writtenPageId = 0;
  char writtenFirstByte = 0;

  void writePage(BufferPoolPageId pageId, BufferPoolPage page) override {
    ++writeCount;
    writtenPageId = pageId;
    writtenFirstByte = page.data()[0];
  }
};

class NewPageTestEvictionPolicy : public EvictionPolicy {
public:
  std::optional<BufferPoolPageId> victim;
  void RecordAccess(BufferPoolPageId) override {}
  void SetEvictable(BufferPoolPageId, bool) override {}
  std::optional<BufferPoolPageId> Evict() override { return victim; }
  void Remove(BufferPoolPageId) override {}
};

TEST(NewPageActionTest, ReusedFrameStartsWithZeroedPageContents) {
  BufferPool bufferPool(1);
  Frame &frame = bufferPool.getFrame(0);
  BufferPoolPage oldPage;
  oldPage.data()[0] = 'A';
  frame.setPage(oldPage);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.setDirty(true);

  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy evictionPolicy;
  evictionPolicy.victim = 1;

  NewPageAction action(bufferPool, allocatePort, writePort, evictionPolicy);
  EXPECT_EQ(action.execute(), 2);

  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(writePort.writtenPageId, 1);
  EXPECT_EQ(writePort.writtenFirstByte, 'A');
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_TRUE(frame.isDirty());
  EXPECT_EQ(frame.getPage().data()[0], 0);
}

TEST(NewPageActionTest, UnoccupiedFrameStartsWithZeroedPageContents) {
  BufferPool bufferPool(1);
  Frame &frame = bufferPool.getFrame(0);
  BufferPoolPage stalePage;
  stalePage.data()[0] = 'X';
  frame.setPage(stalePage);

  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePort, writePort, evictionPolicy);
  EXPECT_EQ(action.execute(), 2);

  EXPECT_EQ(writePort.writeCount, 0);
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_TRUE(frame.isDirty());
  EXPECT_EQ(frame.getPage().data()[0], 0);
}
