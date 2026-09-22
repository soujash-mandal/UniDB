#include "FreeSpaceMapPage.h"

#include <cstring>
#include <stdexcept>

FreeSpaceMapPage::FreeSpaceMapPage()
    : nextPageId(INVALID_PAGE_ID), tableId(0), entryCount(0) {}

PageId FreeSpaceMapPage::getNextPageId() const { return nextPageId; }

void FreeSpaceMapPage::setNextPageId(PageId pageId) { nextPageId = pageId; }

TableId FreeSpaceMapPage::getTableId() const { return tableId; }

void FreeSpaceMapPage::setTableId(TableId tableId) { this->tableId = tableId; }

uint32_t FreeSpaceMapPage::getEntryCount() const { return entryCount; }

bool FreeSpaceMapPage::isFull() const { return entryCount >= MAX_ENTRIES; }

void FreeSpaceMapPage::addEntry(PageId pageId, uint32_t freeSpace) {
  if (isFull()) {
    throw std::runtime_error("FreeSpaceMapPage is full");
  }

  entries[entryCount].pageId = pageId;
  entries[entryCount].freeSpace = freeSpace;

  ++entryCount;
}

void FreeSpaceMapPage::updateEntry(PageId pageId, uint32_t freeSpace) {
  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].pageId == pageId) {
      entries[i].freeSpace = freeSpace;
      return;
    }
  }

  throw std::runtime_error("PageId not found in FreeSpaceMapPage");
}

bool FreeSpaceMapPage::getFreeSpace(PageId pageId, uint32_t &freeSpace) const {
  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].pageId == pageId) {
      freeSpace = entries[i].freeSpace;
      return true;
    }
  }

  return false;
}

PageId FreeSpaceMapPage::findPageWithSpace(uint32_t requiredSpace) const {

  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].freeSpace >= requiredSpace) {
      return entries[i].pageId;
    }
  }

  return INVALID_PAGE_ID;
}

void FreeSpaceMapPage::readFromPage(const Page &page) {
  const char *data = page.data();

  std::memcpy(&nextPageId, data, sizeof(nextPageId));

  std::memcpy(&tableId, data + sizeof(nextPageId), sizeof(tableId));

  std::memcpy(&entryCount, data + sizeof(nextPageId) + sizeof(tableId),
              sizeof(entryCount));

  if (entryCount > MAX_ENTRIES) {
    throw std::runtime_error("Invalid FreeSpaceMapPage entry count");
  }

  std::memcpy(entries, data + HEADER_SIZE, entryCount * sizeof(PageFreeSpace));
}

void FreeSpaceMapPage::writeToPage(Page &page) const {
  char *data = page.data();

  std::memset(data, 0, Page::PAGE_SIZE);

  std::memcpy(data, &nextPageId, sizeof(nextPageId));

  std::memcpy(data + sizeof(nextPageId), &tableId, sizeof(tableId));

  std::memcpy(data + sizeof(nextPageId) + sizeof(tableId), &entryCount,
              sizeof(entryCount));

  std::memcpy(data + HEADER_SIZE, entries, entryCount * sizeof(PageFreeSpace));
}
