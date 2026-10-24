#include "../../adapter/SpaceMapAdapter.h"

#include "../../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
#include "../../../SpaceMapService/actions/FindPageWithSpaceAction.h"
#include "../../../SpaceMapService/actions/GetTablePagesAction.h"
#include "../../../SpaceMapService/domain/SpaceMapPage.h"
#include "../../../SpaceMapService/port/CreateTuplePagePort.h"
#include "../../../SpaceMapService/port/FetchPagePort.h"
#include "../../../SpaceMapService/port/NewPagePort.h"
#include "../../../SpaceMapService/port/UnpinPagePort.h"
#include "../../../SpaceMapService/port/WritePagePort.h"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <vector>

namespace {
class FakeSpaceMapStore final : public NewPagePort,
                                public WritePagePort,
                                public UnpinPagePort,
                                public FetchPagePort,
                                public CreateTuplePagePort {
public:
  SpaceMapPageId newPage() override {
    const auto id = nextPageId++;
    pages.emplace(id, SpaceMapPage{});
    return id;
  }

  void writePage(SpaceMapPageId pageId, SpaceMapPage page) override {
    pages[pageId] = page;
  }

  void unpinPage(SpaceMapPageId pageId) override { unpinned.push_back(pageId); }

  SpaceMapPage fetchPage(SpaceMapPageId pageId) override {
    const auto it = pages.find(pageId);
    if (it == pages.end()) {
      throw std::runtime_error("SpaceMap page not found");
    }
    return it->second;
  }

  CreateTuplePageResult createTuplePage() override {
    return {nextTuplePageId++, 8000};
  }

  std::map<SpaceMapPageId, SpaceMapPage> pages;
  std::vector<SpaceMapPageId> unpinned;

private:
  SpaceMapPageId nextPageId = 20;
  TuplePageId nextTuplePageId = 200;
};
} // namespace

TEST(SpaceMapAdapterTest, DelegatesPageCreationAndTablePageDiscovery) {
  FakeSpaceMapStore store;
  CreateSpaceMapPageAction createAction(store, store, store);
  FindPageWithSpaceAction findAction(store, store, store, store, store);
  GetTablePagesAction getPagesAction(store, store);
  SpaceMapAdapter adapter(createAction, findAction, getPagesAction);

  const auto rootPageId = adapter.createSpaceMapPage();
  auto rootPage = store.pages.at(rootPageId);
  rootPage.insert(200, 7000);
  rootPage.insert(201, 6000);
  store.pages[rootPageId] = rootPage;

  EXPECT_EQ(rootPageId, 20U);
  EXPECT_EQ(adapter.getTablePages(rootPageId),
            (std::vector<uint32_t>{200U, 201U}));
  EXPECT_EQ(store.unpinned, (std::vector<uint32_t>{20U, 20U}));
}
