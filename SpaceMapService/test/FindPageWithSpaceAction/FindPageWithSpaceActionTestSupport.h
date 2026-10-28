#pragma once

#include "../../actions/FindPageWithSpaceAction.h"

#include <gtest/gtest.h>

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

class FindPageWithSpaceTestStore final : public FetchPagePort,
                                         public UnpinPagePort,
                                         public SpaceMapWritePagePort,
                                         public NewPagePort,
                                         public CreateTuplePagePort {
public:
  SpaceMapPage fetchPage(SpaceMapPageId pageId) override {
    const auto it = pages.find(pageId);
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

  void writePage(SpaceMapPageId pageId, SpaceMapPage page) override {
    pages[pageId] = page;
  }

  SpaceMapPageId newPage() override {
    const SpaceMapPageId pageId = nextSpaceMapPageId++;
    pinnedPages.insert(pageId);
    return pageId;
  }

  CreateTuplePageResult createTuplePage() override {
    ++createTuplePageCalls;
    return tuplePageToCreate;
  }

  void seed(SpaceMapPageId pageId, const SpaceMapPage &page) {
    pages[pageId] = page;
  }

  std::map<SpaceMapPageId, SpaceMapPage> pages;
  std::set<SpaceMapPageId> pinnedPages;
  std::vector<SpaceMapPageId> unpinnedPages;
  SpaceMapPageId nextSpaceMapPageId = 1000;
  CreateTuplePageResult tuplePageToCreate{300, 7000};
  int createTuplePageCalls = 0;
};
