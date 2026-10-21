#pragma once

#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <map>
#include <stdexcept>

class InMemoryTuplePageStore : public TupleFetchPagePort,
                               public TupleNewPagePort,
                               public TupleWritePagePort,
                               public TupleUnpinPagePort {
public:
  TuplePage fetchPage(TuplePageId pageId) override {
    if (failFetch) {
      failFetch = false;
      throw std::runtime_error("Injected fetch failure");
    }
    auto it = pages.find(pageId);
    if (it == pages.end()) {
      throw std::runtime_error("Tuple page not found");
    }
    ++pins[pageId];
    return it->second;
  }

  TuplePageId newPage() override {
    if (failAllocation) {
      failAllocation = false;
      throw std::runtime_error("Injected allocation failure");
    }
    const TuplePageId pageId = nextPageId++;
    pages.emplace(pageId, TuplePage{});
    ++pins[pageId];
    return pageId;
  }

  void writePage(TuplePageId pageId, TuplePage page) override {
    if (failWrite) {
      failWrite = false;
      throw std::runtime_error("Injected write failure");
    }
    pages.insert_or_assign(pageId, page);
  }

  void unpinPage(TuplePageId pageId) override {
    auto it = pins.find(pageId);
    if (it == pins.end() || it->second == 0) {
      throw std::runtime_error("Unpin without matching pin");
    }
    --it->second;
  }

  void seed(TuplePageId pageId, const TuplePage &page) {
    pages.insert_or_assign(pageId, page);
    if (pageId >= nextPageId) {
      nextPageId = pageId + 1;
    }
  }

  bool hasPinnedPages() const {
    for (const auto &entry : pins) {
      if (entry.second != 0) {
        return true;
      }
    }
    return false;
  }

  std::map<TuplePageId, TuplePage> pages;
  std::map<TuplePageId, uint32_t> pins;
  bool failFetch = false;
  bool failAllocation = false;
  bool failWrite = false;

private:
  TuplePageId nextPageId = 1;
};
