#include "../actions/DropTupleTableAction.h"

#include <gtest/gtest.h>

#include <limits>
#include <unordered_map>
#include <vector>

namespace {
class TupleStore final : public TupleFetchPagePort, public TupleUnpinPagePort {
public:
  TuplePage fetchPage(TuplePageId pageId) override { return pages.at(pageId); }
  void unpinPage(TuplePageId pageId) override { unpinned.push_back(pageId); }

  std::unordered_map<TuplePageId, TuplePage> pages;
  std::vector<TuplePageId> unpinned;
};

class FreePageRecorder final : public FreeTuplePagePort {
public:
  void freePage(uint32_t pageId) override { freed.push_back(pageId); }
  std::vector<uint32_t> freed;
};
} // namespace

TEST(DropTupleTableActionTest, FreesEveryPageInTheChainAfterUnpinning) {
  TupleStore store;
  FreePageRecorder freePages;
  TuplePage first;
  first.setNextPageId(12);
  TuplePage second;
  second.setNextPageId(std::numeric_limits<TuplePageId>::max());
  store.pages.emplace(11, first);
  store.pages.emplace(12, second);

  DropTupleTableAction action(store, store, freePages);
  action.execute(11);

  EXPECT_EQ(store.unpinned, (std::vector<TuplePageId>{11, 12}));
  EXPECT_EQ(freePages.freed, (std::vector<uint32_t>{11, 12}));
}

TEST(DropTupleTableActionTest, EmptyTableDoesNotFetchOrFreePages) {
  TupleStore store;
  FreePageRecorder freePages;
  DropTupleTableAction action(store, store, freePages);

  action.execute(std::numeric_limits<uint32_t>::max());

  EXPECT_TRUE(store.unpinned.empty());
  EXPECT_TRUE(freePages.freed.empty());
}
