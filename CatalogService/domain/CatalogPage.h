#pragma once

#include <cstdint>
#include <vector>

#include "../../core/Page.h"
#include "../../core/PageId.h"

class CatalogPage {
public:
  static constexpr PageId INVALID_PAGE_ID = UINT32_MAX;

  struct Slot {
    uint16_t offset;
    uint16_t length;
  };

  CatalogPage();

  PageId getNextPageId() const;
  void setNextPageId(PageId pageId);

  uint16_t getTableCount() const;

  bool canFitTable(uint16_t tableSize) const;

  bool addTable(const std::vector<uint8_t> &tableData);

  bool getTable(uint16_t slotIndex, std::vector<uint8_t> &tableData) const;

  bool deleteTable(uint16_t slotIndex);

  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;

private:
  static constexpr uint16_t HEADER_SIZE =
      sizeof(PageId) + sizeof(uint16_t) + sizeof(uint16_t);

  static constexpr uint16_t SLOT_SIZE = sizeof(uint16_t) + sizeof(uint16_t);

  PageId nextPageId;
  uint16_t tableCount;
  uint16_t freeSpaceOffset;

  std::vector<Slot> slots;
  std::vector<uint8_t> data;

  uint16_t getFreeSpace() const;
};
