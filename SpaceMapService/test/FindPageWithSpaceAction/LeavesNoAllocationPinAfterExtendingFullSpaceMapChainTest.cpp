#include "../../actions/FindPageWithSpaceAction.h"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <unordered_map>

class PinCountingSpaceMapStore final : public FetchPagePort,
                                       public UnpinPagePort,
                                       public WritePagePort,
                                       public NewPagePort,
                                       public CreateTuplePagePort {
public:
  SpaceMapPage fetchPage(SpaceMapPageId pageId) override {
    const auto it = pages.find(pageId);
    if (it == pages.end()) {
      throw std::runtime_error("SpaceMap page not found");
    }
    ++pinCounts[pageId];
    return it->second;
  }

  void unpinPage(SpaceMapPageId pageId) override {
    auto it = pinCounts.find(pageId);
    if (it == pinCounts.end() || it->second == 0) {
      throw std::runtime_error("SpaceMap page is not pinned");
    }
    --it->second;
  }

  void writePage(SpaceMapPageId pageId, SpaceMapPage page) override {
    pages[pageId] = page;
  }

  SpaceMapPageId newPage() override {
    ++pinCounts[nextPageId];
    return nextPageId++;
  }

  CreateTuplePageResult createTuplePage() override {
    return {300, 7000};
  }

  std::map<SpaceMapPageId, SpaceMapPage> pages;
  std::unordered_map<SpaceMapPageId, unsigned int> pinCounts;
  SpaceMapPageId nextPageId = 1000;
};

TEST(FindPageWithSpaceActionTest,
     LeavesNoAllocationPinAfterExtendingFullSpaceMapChain) {
  PinCountingSpaceMapStore store;
  SpaceMapPage fullRoot;
  TuplePageId tuplePageId = 1;
  while (!fullRoot.isFull()) {
    fullRoot.insert(tuplePageId++, 0);
  }
  store.pages.emplace(10, fullRoot);

  FindPageWithSpaceAction action(store, store, store, store, store);
  const FindPageWithSpaceResult result = action.execute(10, 128);

  EXPECT_EQ(result.spaceMapPageId, 1000);
  EXPECT_EQ(result.tuplePageId, 300);
  EXPECT_EQ(store.pinCounts[10], 0);
  EXPECT_EQ(store.pinCounts[1000], 0);
}
