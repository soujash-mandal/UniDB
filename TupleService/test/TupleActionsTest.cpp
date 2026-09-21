#include <cstdint>
#include <gtest/gtest.h>

#include <string>

#include "../../DiskManager/adapter/FileDiskManagerAdapter.h"
#include "../../container/container.h"
#include "../../core/Page.h"
#include "../actions/CreatePageAction.h"
#include "../actions/CreateTupleAction.h"
#include "../actions/DeleteTupleAction.h"
#include "../actions/GetTupleAction.h"

class TupleActionsTest : public ::testing::Test {
protected:
  FileDiskManagerAdapter *diskManagerAdapter;
  Container *container;

  const PageId pageId = 0;

  void SetUp() override {
    diskManagerAdapter = new FileDiskManagerAdapter("tuple_test.db");
    uint32_t bufferPoolSize = 100;

    container = new Container(*diskManagerAdapter, bufferPoolSize,
                              EvictionPolicyType::FIFO);

    container->createPageAction().execute(pageId);
  }

  void TearDown() override {
    delete container;
    delete diskManagerAdapter;
  }
};

// --------------------------------------------------
// CREATE 5 TUPLES
// --------------------------------------------------

TEST_F(TupleActionsTest, CreateFiveTuples) {

  const std::string tuple1 = "Tuple One";
  const std::string tuple2 = "Tuple Two";
  const std::string tuple3 = "Tuple Three";
  const std::string tuple4 = "Tuple Four";
  const std::string tuple5 = "Tuple Five";

  const uint16_t slotId1 = container->createTupleAction().execute(
      pageId, tuple1.data(), static_cast<uint16_t>(tuple1.size()));

  const uint16_t slotId2 = container->createTupleAction().execute(
      pageId, tuple2.data(), static_cast<uint16_t>(tuple2.size()));

  const uint16_t slotId3 = container->createTupleAction().execute(
      pageId, tuple3.data(), static_cast<uint16_t>(tuple3.size()));

  const uint16_t slotId4 = container->createTupleAction().execute(
      pageId, tuple4.data(), static_cast<uint16_t>(tuple4.size()));

  const uint16_t slotId5 = container->createTupleAction().execute(
      pageId, tuple5.data(), static_cast<uint16_t>(tuple5.size()));

  EXPECT_EQ(slotId1, 0);
  EXPECT_EQ(slotId2, 1);
  EXPECT_EQ(slotId3, 2);
  EXPECT_EQ(slotId4, 3);
  EXPECT_EQ(slotId5, 4);
}

// --------------------------------------------------
// GET TUPLE
// --------------------------------------------------

TEST_F(TupleActionsTest, GetTuple) {

  const std::string tuple = "Hello from UniDB";

  const uint16_t slotId = container->createTupleAction().execute(
      pageId, tuple.data(), static_cast<uint16_t>(tuple.size()));

  char tupleData[Page::PAGE_SIZE]{};

  container->getTupleAction().execute(pageId, slotId, tupleData);

  const std::string result(tupleData, tuple.size());

  EXPECT_EQ(result, tuple);
}

// --------------------------------------------------
// GET MULTIPLE TUPLES
// --------------------------------------------------

TEST_F(TupleActionsTest, GetMultipleTuples) {

  const std::string tuple1 = "Tuple One";
  const std::string tuple2 = "Tuple Two";
  const std::string tuple3 = "Tuple Three";

  const uint16_t slotId1 = container->createTupleAction().execute(
      pageId, tuple1.data(), static_cast<uint16_t>(tuple1.size()));

  const uint16_t slotId2 = container->createTupleAction().execute(
      pageId, tuple2.data(), static_cast<uint16_t>(tuple2.size()));

  const uint16_t slotId3 = container->createTupleAction().execute(
      pageId, tuple3.data(), static_cast<uint16_t>(tuple3.size()));

  char data1[Page::PAGE_SIZE]{};
  char data2[Page::PAGE_SIZE]{};
  char data3[Page::PAGE_SIZE]{};

  container->getTupleAction().execute(pageId, slotId1, data1);

  container->getTupleAction().execute(pageId, slotId2, data2);

  container->getTupleAction().execute(pageId, slotId3, data3);

  EXPECT_EQ(std::string(data1, tuple1.size()), tuple1);

  EXPECT_EQ(std::string(data2, tuple2.size()), tuple2);

  EXPECT_EQ(std::string(data3, tuple3.size()), tuple3);
}

// --------------------------------------------------
// DELETE TUPLE
// --------------------------------------------------

TEST_F(TupleActionsTest, DeleteTuple) {

  const std::string tuple = "Tuple To Delete";

  const uint16_t slotId = container->createTupleAction().execute(
      pageId, tuple.data(), static_cast<uint16_t>(tuple.size()));

  container->deleteTupleAction().execute(pageId, slotId);

  char tupleData[Page::PAGE_SIZE]{};

  EXPECT_THROW(container->getTupleAction().execute(pageId, slotId, tupleData),
               std::exception);
}

// --------------------------------------------------
// DELETE ONE TUPLE, OTHERS REMAIN
// --------------------------------------------------

TEST_F(TupleActionsTest, DeleteOneTupleOthersRemain) {

  const std::string tuple1 = "Tuple One";
  const std::string tuple2 = "Tuple Two";
  const std::string tuple3 = "Tuple Three";

  const uint16_t slotId1 = container->createTupleAction().execute(
      pageId, tuple1.data(), static_cast<uint16_t>(tuple1.size()));

  const uint16_t slotId2 = container->createTupleAction().execute(
      pageId, tuple2.data(), static_cast<uint16_t>(tuple2.size()));

  const uint16_t slotId3 = container->createTupleAction().execute(
      pageId, tuple3.data(), static_cast<uint16_t>(tuple3.size()));

  // Delete tuple 2

  container->deleteTupleAction().execute(pageId, slotId2);

  // Tuple 1 should still exist

  char data1[Page::PAGE_SIZE]{};

  container->getTupleAction().execute(pageId, slotId1, data1);

  EXPECT_EQ(std::string(data1, tuple1.size()), tuple1);

  // Tuple 2 should not exist

  char data2[Page::PAGE_SIZE]{};

  EXPECT_THROW(container->getTupleAction().execute(pageId, slotId2, data2),
               std::exception);

  // Tuple 3 should still exist

  char data3[Page::PAGE_SIZE]{};

  container->getTupleAction().execute(pageId, slotId3, data3);

  EXPECT_EQ(std::string(data3, tuple3.size()), tuple3);
}