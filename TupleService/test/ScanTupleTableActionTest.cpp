#include "../actions/ScanTupleTableAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstring>
#include <stdexcept>
#include <vector>

TEST(ScanTupleTableActionTest, ScansSuppliedPagesInOrderAndSkipsDeletedSlots) {
  InMemoryTuplePageStore store;
  TuplePage first;
  first.insert({'a'});
  first.insert({'x'});
  first.remove(1);
  TuplePage second;
  second.insert({'b'});
  store.seed(12, first);
  store.seed(42, second);
  ScanTupleTableAction action(store, store);

  EXPECT_EQ(action.execute({12, 42}),
            (std::vector<std::vector<char>>{{'a'}, {'b'}}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, ReturnsEmptyForEmptyPageList) {
  InMemoryTuplePageStore store;
  ScanTupleTableAction action(store, store);

  EXPECT_TRUE(action.execute({}).empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, ReturnsEmptyForEmptyPages) {
  InMemoryTuplePageStore store;
  store.seed(12, TuplePage{});
  ScanTupleTableAction action(store, store);

  EXPECT_TRUE(action.execute({12}).empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, UnpinsPageWhenTupleMaterializationFails) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'x'});
  const uint32_t invalidOffset = 8192;
  std::memcpy(page.data() + sizeof(uint32_t) * 2, &invalidOffset,
              sizeof(invalidOffset));
  store.seed(12, page);
  ScanTupleTableAction action(store, store);

  EXPECT_THROW(action.execute({12}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  ScanTupleTableAction action(store, store);

  EXPECT_THROW(action.execute({12}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
