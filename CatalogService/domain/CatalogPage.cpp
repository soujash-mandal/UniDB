#include "CatalogPage.h"

#include <cstring>

CatalogPage::CatalogPage()
    : nextPageId(INVALID_PAGE_ID), tableCount(0),
      freeSpaceOffset(Page::PAGE_SIZE), slots(), data(Page::PAGE_SIZE, 0) {}

PageId CatalogPage::getNextPageId() const { return nextPageId; }

void CatalogPage::setNextPageId(PageId pageId) { nextPageId = pageId; }

uint16_t CatalogPage::getTableCount() const { return tableCount; }

uint16_t CatalogPage::getFreeSpace() const {
  const uint16_t slotDirectoryEnd =
      HEADER_SIZE + static_cast<uint16_t>(slots.size() * SLOT_SIZE);

  if (freeSpaceOffset <= slotDirectoryEnd) {
    return 0;
  }

  return freeSpaceOffset - slotDirectoryEnd;
}

bool CatalogPage::canFitTable(uint16_t tableSize) const {
  return getFreeSpace() >= tableSize + SLOT_SIZE;
}

bool CatalogPage::addTable(const std::vector<uint8_t> &tableData) {

  if (tableData.empty()) {
    return false;
  }

  if (tableData.size() > UINT16_MAX) {
    return false;
  }

  const uint16_t tableSize = static_cast<uint16_t>(tableData.size());

  /*
   * First try to reuse a deleted slot.
   *
   * A deleted slot has length == 0.
   */
  for (Slot &slot : slots) {
    if (slot.length != 0) {
      continue;
    }

    if (freeSpaceOffset < tableSize) {
      return false;
    }

    const uint16_t newOffset =
        static_cast<uint16_t>(freeSpaceOffset - tableSize);

    std::memcpy(data.data() + newOffset, tableData.data(), tableSize);

    slot.offset = newOffset;
    slot.length = tableSize;

    freeSpaceOffset = newOffset;

    return true;
  }

  /*
   * No deleted slot exists.
   * We need space for both the table and a new slot.
   */
  if (!canFitTable(tableSize)) {
    return false;
  }

  freeSpaceOffset = static_cast<uint16_t>(freeSpaceOffset - tableSize);

  std::memcpy(data.data() + freeSpaceOffset, tableData.data(), tableSize);

  slots.push_back({freeSpaceOffset, tableSize});

  tableCount = static_cast<uint16_t>(slots.size());

  return true;
}

bool CatalogPage::getTable(uint16_t slotIndex,
                           std::vector<uint8_t> &tableData) const {

  if (slotIndex >= slots.size()) {
    return false;
  }

  const Slot &slot = slots[slotIndex];

  /*
   * length == 0 means the slot was deleted.
   */
  if (slot.length == 0) {
    return false;
  }

  tableData.resize(slot.length);

  std::memcpy(tableData.data(), data.data() + slot.offset, slot.length);

  return true;
}

bool CatalogPage::deleteTable(uint16_t slotIndex) {
  if (slotIndex >= slots.size()) {
    return false;
  }

  Slot &slot = slots[slotIndex];

  if (slot.length == 0) {
    return false;
  }

  /*
   * Do not remove the slot.
   *
   * Keeping the slot preserves slot indexes.
   * length == 0 means "deleted".
   */
  slot.length = 0;

  return true;
}

void CatalogPage::readFromPage(const Page &page) {
  const char *pageData = page.data();

  std::memcpy(&nextPageId, pageData, sizeof(nextPageId));

  std::memcpy(&tableCount, pageData + sizeof(nextPageId), sizeof(tableCount));

  std::memcpy(&freeSpaceOffset,
              pageData + sizeof(nextPageId) + sizeof(tableCount),
              sizeof(freeSpaceOffset));

  slots.clear();

  uint16_t slotOffset = HEADER_SIZE;

  for (uint16_t i = 0; i < tableCount; ++i) {
    Slot slot{};

    std::memcpy(&slot.offset, pageData + slotOffset, sizeof(slot.offset));

    slotOffset += sizeof(slot.offset);

    std::memcpy(&slot.length, pageData + slotOffset, sizeof(slot.length));

    slotOffset += sizeof(slot.length);

    slots.push_back(slot);
  }

  data.resize(Page::PAGE_SIZE);

  std::memcpy(data.data(), pageData, Page::PAGE_SIZE);
}

void CatalogPage::writeToPage(Page &page) const {
  std::memset(page.data(), 0, Page::PAGE_SIZE);
  char *pageData = page.data();

  /*
   * Header
   */
  std::memcpy(pageData, &nextPageId, sizeof(nextPageId));

  std::memcpy(pageData + sizeof(nextPageId), &tableCount, sizeof(tableCount));

  std::memcpy(pageData + sizeof(nextPageId) + sizeof(tableCount),
              &freeSpaceOffset, sizeof(freeSpaceOffset));

  /*
   * Slot directory
   */
  uint16_t slotOffset = HEADER_SIZE;

  for (const Slot &slot : slots) {
    std::memcpy(pageData + slotOffset, &slot.offset, sizeof(slot.offset));

    slotOffset += sizeof(slot.offset);

    std::memcpy(pageData + slotOffset, &slot.length, sizeof(slot.length));

    slotOffset += sizeof(slot.length);
  }

  /*
   * Table records.
   */
  if (freeSpaceOffset < Page::PAGE_SIZE) {
    std::memcpy(pageData + freeSpaceOffset, data.data() + freeSpaceOffset,
                Page::PAGE_SIZE - freeSpaceOffset);
  }
}
