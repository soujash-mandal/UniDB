#include "../actions/ScanTupleTableAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstring>
#include <stdexcept>
#include <vector>

TEST(ScanTupleTableActionTest, ScansTuplesInSlotOrderAndSkipsDeletedSlots) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'a'});
  page.insert({'b'});
  page.remove(0);
  store.seed(12, page);
  ScanTupleTableAction action(store, store);

  EXPECT_EQ(action.execute(12), (std::vector<std::vector<char>>{{'b'}}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, ScansLinkedPagesInOrder) {
  InMemoryTuplePageStore store;
  TuplePage first;
  first.insert({'a'});
  first.setNextPageId(13);
  TuplePage second;
  second.insert({'b'});
  store.seed(12, first);
  store.seed(13, second);
  ScanTupleTableAction action(store, store);

  EXPECT_EQ(action.execute(12), (std::vector<std::vector<char>>{{'a'}, {'b'}}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, ReturnsEmptyForEmptyPage) {
  InMemoryTuplePageStore store;
  store.seed(12, TuplePage{});
  ScanTupleTableAction action(store, store);

  EXPECT_TRUE(action.execute(12).empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, RejectsCyclicPageChainAndUnpinsPages) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'a'});
  page.setNextPageId(12);
  store.seed(12, page);
  ScanTupleTableAction action(store, store);

  EXPECT_THROW(action.execute(12), std::runtime_error);
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

  EXPECT_THROW(action.execute(12), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(ScanTupleTableActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  ScanTupleTableAction action(store, store);

  EXPECT_THROW(action.execute(12), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
