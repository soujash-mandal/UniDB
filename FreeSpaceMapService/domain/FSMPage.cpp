#include "FSMPage.h"

#include <cstring>
#include <stdexcept>

FSMPage::FSMPage() {
  std::memset(bytes, 0, PAGE_SIZE);
  auto *header = reinterpret_cast<Header *>(bytes);
  header->nextPageId = INVALID_PAGE_ID;
  header->entryCount = 0;
}

FSMPageId FSMPage::getNextPageId() {
  auto *header = reinterpret_cast<Header *>(bytes);
  return header->nextPageId;
}

void FSMPage::setNextPageId(FSMPageId pageId) {
  auto *header = reinterpret_cast<Header *>(bytes);
  header->nextPageId = pageId;
}

uint32_t FSMPage::getEntryCount() {
  auto *header = reinterpret_cast<Header *>(bytes);
  return header->entryCount;
}

bool FSMPage::isFull() { return getEntryCount() >= MAX_ENTRIES; }

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
  uint32_t entryCount = getEntryCount();
  auto *entries = reinterpret_cast<Entry *>(bytes + HEADER_SIZE);
  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].pageId == pageId) {
      entries[i].freeSpace = freeSpace;
      return;
    }
  }
  throw std::runtime_error("PageId not found in FSMPage");
}

FSMPageId FSMPage::findPageWithSpace(uint32_t requiredSpace) {
  uint32_t entryCount = getEntryCount();
  auto *entries = reinterpret_cast<Entry *>(bytes + HEADER_SIZE);
  for (uint32_t i = 0; i < entryCount; ++i) {
    if (entries[i].freeSpace >= requiredSpace) {
      return entries[i].pageId;
    }
  }
  return INVALID_PAGE_ID;
}

char *FSMPage::data() { return bytes; }

const char *FSMPage::data() const { return bytes; }
