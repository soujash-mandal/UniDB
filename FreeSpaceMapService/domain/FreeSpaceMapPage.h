#pragma once

#include <cstdint>

#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "PageFreeSpace.h"

class FreeSpaceMapPage {
public:
  static constexpr PageId INVALID_PAGE_ID = UINT32_MAX;
  static constexpr uint32_t HEADER_SIZE =
      sizeof(PageId) + sizeof(TableId) + sizeof(uint32_t);
  static constexpr uint32_t MAX_ENTRIES =
      (Page::PAGE_SIZE - HEADER_SIZE) / sizeof(PageFreeSpace);

private:
  PageId nextPageId;
  TableId tableId;
  uint32_t entryCount;
  PageFreeSpace entries[MAX_ENTRIES];

public:
  FreeSpaceMapPage();

  PageId getNextPageId() const;
  void setNextPageId(PageId pageId);

  TableId getTableId() const;
  void setTableId(TableId tableId);

  uint32_t getEntryCount() const;
  bool isFull() const;

  void addEntry(PageId pageId, uint32_t freeSpace);
  void updateEntry(PageId pageId, uint32_t freeSpace);

  bool getFreeSpace(PageId pageId, uint32_t &freeSpace) const;

  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;
};
