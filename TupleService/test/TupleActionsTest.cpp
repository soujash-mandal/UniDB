#include "../actions/CreateTupleAction.h"
#include "../actions/DeleteTupleAction.h"
#include "../actions/GetTupleAction.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

class TupleTestFetchPort : public FetchPagePort {
public:
  TuplePage page;
  int fetchCount = 0;

  TuplePage fetchPage(TuplePageId) override {
    ++fetchCount;
    return page;
  }
};

class TupleTestWritePort : public WritePagePort {
public:
  int writeCount = 0;
  void writePage(TuplePageId, TuplePage) override { ++writeCount; }
};

class TupleTestUnpinPort : public UnpinPagePort {
public:
  int unpinCount = 0;
  TuplePageId lastPageId = 0;

  void unpinPage(TuplePageId pageId) override {
    ++unpinCount;
    lastPageId = pageId;
  }
};

TEST(TupleActionsTest, CreateTupleUnpinsAfterSuccessfulWrite) {
  TupleTestFetchPort fetchPort;
  TupleTestWritePort writePort;
  TupleTestUnpinPort unpinPort;
  CreateTupleAction action(fetchPort, writePort, unpinPort);

  EXPECT_EQ(action.execute(7, {'a', 'b'}), 0);
  EXPECT_EQ(fetchPort.fetchCount, 1);
  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(unpinPort.unpinCount, 1);
  EXPECT_EQ(unpinPort.lastPageId, 7);
}

TEST(TupleActionsTest, CreateTupleUnpinsWhenInsertFails) {
  TupleTestFetchPort fetchPort;
  TupleTestWritePort writePort;
  TupleTestUnpinPort unpinPort;
  CreateTupleAction action(fetchPort, writePort, unpinPort);

  EXPECT_THROW(action.execute(7, std::vector<char>(8192, 'x')),
               std::runtime_error);
  EXPECT_EQ(writePort.writeCount, 0);
  EXPECT_EQ(unpinPort.unpinCount, 1);
}

TEST(TupleActionsTest, DeleteTupleUnpinsAfterSuccessfulWrite) {
  TupleTestFetchPort fetchPort;
  fetchPort.page.insert({'x'});
  TupleTestWritePort writePort;
  TupleTestUnpinPort unpinPort;
  DeleteTupleAction action(fetchPort, writePort, unpinPort);

  action.execute(9, 0);
  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(unpinPort.unpinCount, 1);
  EXPECT_EQ(unpinPort.lastPageId, 9);
}

TEST(TupleActionsTest, DeleteTupleUnpinsWhenSlotIsInvalid) {
  TupleTestFetchPort fetchPort;
  TupleTestWritePort writePort;
  TupleTestUnpinPort unpinPort;
  DeleteTupleAction action(fetchPort, writePort, unpinPort);

  EXPECT_THROW(action.execute(9, 0), std::runtime_error);
  EXPECT_EQ(writePort.writeCount, 0);
  EXPECT_EQ(unpinPort.unpinCount, 1);
}

TEST(TupleActionsTest, GetTupleUnpinsWhenSlotIsInvalid) {
  TupleTestFetchPort fetchPort;
  TupleTestUnpinPort unpinPort;
  GetTupleAction action(fetchPort, unpinPort);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_EQ(unpinPort.unpinCount, 1);
  EXPECT_EQ(unpinPort.lastPageId, 11);
}

TEST(TupleActionsTest, GetTupleUnpinsAfterSuccessfulRead) {
  TupleTestFetchPort fetchPort;
  fetchPort.page.insert({'o', 'k'});
  TupleTestUnpinPort unpinPort;
  GetTupleAction action(fetchPort, unpinPort);

  EXPECT_EQ(action.execute(11, 0), (std::vector<char>{'o', 'k'}));
  EXPECT_EQ(unpinPort.unpinCount, 1);
}
