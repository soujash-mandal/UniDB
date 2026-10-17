#include "../actions/InsertTupleIntoTableAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <vector>

TEST(InsertTupleIntoTableActionTest, CreatesFirstPageForEmptyTable) {
  InMemoryTuplePageStore store;
  InsertTupleIntoTableAction action(store, store, store, store);

  const auto result =
      action.execute(std::numeric_limits<TuplePageId>::max(), {'a', 'b'});

  EXPECT_EQ(result.firstPageId, result.pageId);
  EXPECT_EQ(store.pages.at(result.pageId).get(result.slotId),
            (std::vector<char>{'a', 'b'}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(InsertTupleIntoTableActionTest, ReusesExistingPageWithEnoughSpace) {
  InMemoryTuplePageStore store;
  store.seed(4, TuplePage{});
  InsertTupleIntoTableAction action(store, store, store, store);

  const auto result = action.execute(4, {'r'});

  EXPECT_EQ(result.firstPageId, 4U);
  EXPECT_EQ(result.pageId, 4U);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(InsertTupleIntoTableActionTest, AllocatesAndLinksNewPageWhenTailIsFull) {
  InMemoryTuplePageStore store;
  TuplePage fullPage;
  fullPage.insert(std::vector<char>(6000, 'a'));
  store.seed(4, fullPage);
  InsertTupleIntoTableAction action(store, store, store, store);

  const auto result = action.execute(4, std::vector<char>(2500, 'b'));

  EXPECT_EQ(result.firstPageId, 4U);
  EXPECT_NE(result.pageId, 4U);
  EXPECT_EQ(store.pages.at(4).getNextPageId(), result.pageId);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(InsertTupleIntoTableActionTest, RejectsEmptyOrOversizedTupleBeforeAllocation) {
  InMemoryTuplePageStore store;
  InsertTupleIntoTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(std::numeric_limits<TuplePageId>::max(), {}),
               std::invalid_argument);
  EXPECT_THROW(action.execute(std::numeric_limits<TuplePageId>::max(),
                              std::vector<char>(8192, 'x')),
               std::length_error);
  EXPECT_TRUE(store.pages.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(InsertTupleIntoTableActionTest, CleansUpWhenNewPageAllocationFails) {
  InMemoryTuplePageStore store;
  TuplePage fullPage;
  fullPage.insert(std::vector<char>(6000, 'a'));
  store.seed(4, fullPage);
  store.failAllocation = true;
  InsertTupleIntoTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(4, std::vector<char>(2500, 'b')),
               std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(InsertTupleIntoTableActionTest, RejectsCyclicPageChainAndUnpinsVisitedPage) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert(std::vector<char>(6000, 'a'));
  page.setNextPageId(4);
  store.seed(4, page);
  InsertTupleIntoTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(4, std::vector<char>(2500, 'b')),
               std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(InsertTupleIntoTableActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  InsertTupleIntoTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(4, {'x'}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
