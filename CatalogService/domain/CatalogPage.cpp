#include "CatalogPage.h"

#include <cstring>

CatalogPage::CatalogPage()
    : nextPageId(INVALID_PAGE_ID), tableCount(0), freeSpaceOffset(0), data{} {}

PageId CatalogPage::getNextPageId() const { return nextPageId; }

void CatalogPage::setNextPageId(PageId pageId) { nextPageId = pageId; }

uint16_t CatalogPage::getTableCount() const { return tableCount; }

bool CatalogPage::canFitTable(uint16_t tableSize) const {
  return static_cast<uint32_t>(freeSpaceOffset) + tableSize <= sizeof(data);
}

void CatalogPage::writeTable(const char *tableData, uint16_t size) {
  if (!canFitTable(size)) {
    return;
  }

  std::memcpy(data + freeSpaceOffset, tableData, size);

  freeSpaceOffset += size;
  ++tableCount;
}

void CatalogPage::readTable(uint16_t index, char *tableData,
                            uint16_t size) const {
  if (index >= tableCount) {
    return;
  }

  // Temporary implementation.
  // Table offsets will be added when the catalog record format is defined.
  std::memcpy(tableData, data, size);
}

void CatalogPage::readFromPage(const Page &page) {
  std::memcpy(&nextPageId, page.data(), sizeof(PageId));

  std::memcpy(&tableCount, page.data() + sizeof(PageId), sizeof(uint16_t));

  std::memcpy(&freeSpaceOffset, page.data() + sizeof(PageId) + sizeof(uint16_t),
              sizeof(uint16_t));

  std::memcpy(
      data, page.data() + sizeof(PageId) + sizeof(uint16_t) + sizeof(uint16_t),
      sizeof(data));
}

void CatalogPage::writeToPage(Page &page) const {
  std::memset(page.data(), 0, Page::PAGE_SIZE);

  std::memcpy(page.data(), &nextPageId, sizeof(PageId));

  std::memcpy(page.data() + sizeof(PageId), &tableCount, sizeof(uint16_t));

  std::memcpy(page.data() + sizeof(PageId) + sizeof(uint16_t), &freeSpaceOffset,
              sizeof(uint16_t));

  std::memcpy(page.data() + sizeof(PageId) + sizeof(uint16_t) +
                  sizeof(uint16_t),
              data, sizeof(data));
}
