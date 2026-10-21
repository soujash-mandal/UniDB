#pragma once

#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

inline CatalogPage makeCatalogPageWithoutRoomForAnotherTable() {
  CatalogPage page;
  Table wideTable{};
  wideTable.tableId = 1;
  wideTable.name = "wide";
  for (uint32_t columnId = 0; columnId < 203; ++columnId) {
    Column column{};
    column.name = std::string(32, static_cast<char>('a' + columnId % 26));
    column.type = DataType::INT;
    column.size = 0;
    column.nullable = false;
    wideTable.columns.push_back(column);
  }
  page.insert(wideTable);
  return page;
}

class InMemoryCatalogPageStore : public CatalogFetchPagePort,
                                 public CatalogNewPagePort,
                                 public CatalogWritePagePort,
                                 public CatalogUnpinPagePort {
public:
  CatalogPage fetchPage(CatalogPageId pageId) override {
    if (failNextFetch) {
      failNextFetch = false;
      throw std::runtime_error("Injected catalog fetch failure");
    }
    const auto page = pages.find(pageId);
    if (page == pages.end()) {
      throw std::runtime_error("Catalog page not found");
    }
    ++pinCounts[pageId];
    return page->second;
  }

  CatalogPageId newPage() override {
    if (failNextAllocation) {
      failNextAllocation = false;
      throw std::runtime_error("Injected catalog page allocation failure");
    }
    const CatalogPageId pageId = nextPageId++;
    pages.emplace(pageId, CatalogPage{});
    ++pinCounts[pageId];
    return pageId;
  }

  void writePage(CatalogPageId pageId, CatalogPage page) override {
    if (failNextWrite) {
      failNextWrite = false;
      throw std::runtime_error("Injected catalog page write failure");
    }
    pages[pageId] = page;
    writtenPageIds.push_back(pageId);
  }

  void unpinPage(CatalogPageId pageId) override {
    auto pin = pinCounts.find(pageId);
    if (pin == pinCounts.end() || pin->second == 0) {
      throw std::runtime_error("Catalog page unpinned without a matching pin");
    }
    --pin->second;
    unpinnedPageIds.push_back(pageId);
  }

  bool hasPinnedPages() const {
    for (const auto &pin : pinCounts) {
      if (pin.second != 0) {
        return true;
      }
    }
    return false;
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
  bool failNextFetch = false;
  bool failNextAllocation = false;
  bool failNextWrite = false;

private:
  std::map<CatalogPageId, uint32_t> pinCounts;
  CatalogPageId nextPageId = 1;
};
