#pragma once

#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <map>
#include <stdexcept>
#include <vector>

class InMemoryCatalogPageStore : public CatalogFetchPagePort,
                                 public CatalogNewPagePort,
                                 public CatalogWritePagePort,
                                 public CatalogUnpinPagePort {
public:
  CatalogPage fetchPage(CatalogPageId pageId) override {
    const auto page = pages.find(pageId);
    if (page == pages.end()) {
      throw std::runtime_error("Catalog page not found");
    }
    return page->second;
  }

  CatalogPageId newPage() override {
    const CatalogPageId pageId = nextPageId++;
    pages.emplace(pageId, CatalogPage{});
    return pageId;
  }

  void writePage(CatalogPageId pageId, CatalogPage page) override {
    pages[pageId] = page;
    writtenPageIds.push_back(pageId);
  }

  void unpinPage(CatalogPageId pageId) override {
    unpinnedPageIds.push_back(pageId);
  }

  void seed(CatalogPageId pageId, CatalogPage page) {
    pages[pageId] = page;
    if (pageId >= nextPageId && pageId != CATALOG_INVALID_PAGE_ID) {
      nextPageId = pageId + 1;
    }
  }

  std::map<CatalogPageId, CatalogPage> pages;
  std::vector<CatalogPageId> writtenPageIds;
  std::vector<CatalogPageId> unpinnedPageIds;

private:
  CatalogPageId nextPageId = 1;
};
