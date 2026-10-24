#pragma once

#include "../../actions/GetTablePagesAction.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace {
class InMemorySpaceMapStore final : public FetchPagePort, public UnpinPagePort {
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
