#include "FSMPage.h"

#include <cstring>
#include <stdexcept>

FSMPage::FSMPage() {
  std::memset(bytes, 0, PAGE_SIZE);

  auto *header = reinterpret_cast<Header *>(bytes);
  header->nextPageId = INVALID_PAGE_ID;
  header->tableId = 0;
  header->entryCount = 0;
}

FSMPageId FSMPage::getNextPageId() const {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  return header->nextPageId;
}

void FSMPage::setNextPageId(FSMPageId pageId) {
  auto *header = reinterpret_cast<Header *>(bytes);
  header->nextPageId = pageId;
}

TableId FSMPage::getTableId() const {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  return header->tableId;
}

void FSMPage::setTableId(TableId tableId) {
  auto *header = reinterpret_cast<Header *>(bytes);
  header->tableId = tableId;
}

uint32_t FSMPage::getEntryCount() const {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  return header->entryCount;
}

bool FSMPage::isFull() const { return getEntryCount() >= MAX_ENTRIES; }

void FSMPage::addEntry(FSMPageId pageId, uint32_t freeSpace) {
  auto *header = reinterpret_cast<Header *>(bytes);

  if (isFull()) {
    throw std::runtime_error("FSMPage is full");
  }

  auto *entries = reinterpret_cast<Entry *>(bytes + HEADER_SIZE);

  entries[header->entryCount].pageId = pageId;
  entries[header->entryCount].freeSpace = freeSpace;

  ++header->entryCount;
}

void FSMPage::updateEntry(FSMPageId pageId, uint32_t freeSpace) {
  const uint32_t entryCount = getEntryCount();
  auto *entries = reinterpret_cast<Entry *>(bytes + HEADER_SIZE);

  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].pageId == pageId) {
      entries[i].freeSpace = freeSpace;
      return;
    }
  }

  throw std::runtime_error("PageId not found in FSMPage");
}

bool FSMPage::getFreeSpace(FSMPageId pageId, uint32_t &freeSpace) const {
  const uint32_t entryCount = getEntryCount();
  const auto *entries = reinterpret_cast<const Entry *>(bytes + HEADER_SIZE);

  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].pageId == pageId) {
      freeSpace = entries[i].freeSpace;
      return true;
    }
  }

  return false;
}

FSMPageId FSMPage::findPageWithSpace(uint32_t requiredSpace) const {
  const uint32_t entryCount = getEntryCount();
  const auto *entries = reinterpret_cast<const Entry *>(bytes + HEADER_SIZE);

  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].freeSpace >= requiredSpace) {
      return entries[i].pageId;
    }
  }

  return INVALID_PAGE_ID;
}

char *FSMPage::data() { return bytes; }

const char *FSMPage::data() const { return bytes; }
