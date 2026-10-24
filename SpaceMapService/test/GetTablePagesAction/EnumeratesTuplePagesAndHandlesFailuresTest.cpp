#include "../../actions/GetTablePagesAction.h"

#include <gtest/gtest.h>

#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace {
class InMemorySpaceMapStore final : public FetchPagePort,
                                    public UnpinPagePort {
public:
  SpaceMapPage fetchPage(SpaceMapPageId pageId) override {
    auto it = pages.find(pageId);
    if (it == pages.end()) {
      throw std::runtime_error("SpaceMap page not found");
    }
    pinnedPages.insert(pageId);
    return it->second;
  }

  void unpinPage(SpaceMapPageId pageId) override {
    if (pinnedPages.erase(pageId) != 1) {
      throw std::runtime_error("SpaceMap page was not pinned");
    }
    unpinnedPages.push_back(pageId);
  }

  void seed(SpaceMapPageId pageId, const SpaceMapPage &page) {
    pages[pageId] = page;
  }

  bool hasPinnedPages() const { return !pinnedPages.empty(); }

  std::map<SpaceMapPageId, SpaceMapPage> pages;
  std::set<SpaceMapPageId> pinnedPages;
  std::vector<SpaceMapPageId> unpinnedPages;
};
} // namespace

// Scenario 1: Return every tuple page ID from a single SpaceMap page.
TEST(GetTablePagesActionTest, ReturnsTuplePagesFromSingleSpaceMapPage) {
  InMemorySpaceMapStore store;
  SpaceMapPage page;
  page.insert(100, 7000);
  page.insert(101, 6000);
  store.seed(10, page);
  GetTablePagesAction action(store, store);

  EXPECT_EQ(action.execute(10), (std::vector<TuplePageId>{100, 101}));
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}

// Scenario 2: Traverse a chained SpaceMap list and preserve page order.
TEST(GetTablePagesActionTest, TraversesChainedSpaceMapPages) {
  InMemorySpaceMapStore store;
  SpaceMapPage first;
  first.insert(100, 7000);
  first.setNextPageId(11);
  SpaceMapPage second;
  second.insert(200, 5000);
  second.insert(201, 4000);
  store.seed(10, first);
  store.seed(11, second);
  GetTablePagesAction action(store, store);

  EXPECT_EQ(action.execute(10),
            (std::vector<TuplePageId>{100, 200, 201}));
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10, 11}));
  EXPECT_FALSE(store.hasPinnedPages());
}

// Scenario 3: Reject cycles and release the page fetched before detection.
TEST(GetTablePagesActionTest, RejectsCycleWithoutLeakingPins) {
  InMemorySpaceMapStore store;
  SpaceMapPage first;
  first.setNextPageId(11);
  SpaceMapPage second;
  second.setNextPageId(10);
  store.seed(10, first);
  store.seed(11, second);
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10, 11}));
  EXPECT_FALSE(store.hasPinnedPages());
}

// Scenario 4: A failed fetch must not cause an unpin for an unfetched page.
TEST(GetTablePagesActionTest, PropagatesFetchFailureWithoutUnpinningMissingPage) {
  InMemorySpaceMapStore store;
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_TRUE(store.unpinnedPages.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

// Scenario 5: Reject an invalid root before attempting any fetch.
TEST(GetTablePagesActionTest, RejectsInvalidRootPageId) {
  InMemorySpaceMapStore store;
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(UINT32_MAX), std::runtime_error);
  EXPECT_TRUE(store.unpinnedPages.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

// Scenario 6: Reject a corrupt entry count and still unpin the fetched page.
TEST(GetTablePagesActionTest, RejectsCorruptEntryCountAndUnpinsPage) {
  InMemorySpaceMapStore store;
  SpaceMapPage page;
  const uint32_t corruptEntryCount = UINT32_MAX;
  std::memcpy(page.data() + sizeof(SpaceMapPageId), &corruptEntryCount,
              sizeof(corruptEntryCount));
  store.seed(10, page);
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
