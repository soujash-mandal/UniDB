#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

#include <cstdint>

class CatalogPage {
public:
  static constexpr PageId INVALID_PAGE_ID = UINT32_MAX;

  CatalogPage();

  PageId getNextPageId() const;
  void setNextPageId(PageId pageId);

  uint16_t getTableCount() const;

  bool canFitTable(uint16_t tableSize) const;

  void writeTable(const char *data, uint16_t size);

  void readTable(uint16_t index, char *data, uint16_t size) const;

  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;

private:
  PageId nextPageId;
  uint16_t tableCount;
  uint16_t freeSpaceOffset;

  char data[Page::PAGE_SIZE - sizeof(PageId) - sizeof(uint16_t) -
            sizeof(uint16_t)]{};
};
