#include "FSMPage.h"

#include <cstring>
#include <stdexcept>

FSMPage::FSMPage() {
  std::memset(bytes, 0, PAGE_SIZE);
  Header header;
  header.nextPageId = INVALID_PAGE_ID;
  header.entryCount = 0;
  std::memcpy(bytes, &header, sizeof(Header));
}

FSMPageId FSMPage::getNextPageId() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.nextPageId;
}

void FSMPage::setNextPageId(FSMPageId pageId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  header.nextPageId = pageId;
  std::memcpy(bytes, &header, sizeof(Header));
}

bool FSMPage::isFull() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.entryCount >= MAX_ENTRIES;
}

void FSMPage::insert(FSMPageId pageId, uint32_t freeSpace) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  if (header.entryCount >= MAX_ENTRIES) {
    throw std::runtime_error("FSMPage is full");
  }
  Entry entry;
  entry.pageId = pageId;
  entry.freeSpace = freeSpace;
  std::memcpy(bytes + HEADER_SIZE + header.entryCount * sizeof(Entry), &entry,
              sizeof(Entry));
  ++header.entryCount;
  std::memcpy(bytes, &header, sizeof(Header));
}

void FSMPage::update(FSMPageId pageId, uint32_t freeSpace) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + HEADER_SIZE + i * sizeof(Entry), sizeof(Entry));
    if (entry.pageId == pageId) {
      entry.freeSpace = freeSpace;
      std::memcpy(bytes + HEADER_SIZE + i * sizeof(Entry), &entry,
                  sizeof(Entry));
      return;
    }
  }
  throw std::runtime_error("PageId not found in FSMPage");
}

FSMPageId FSMPage::findPageWithSpace(uint32_t requiredSpace) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + HEADER_SIZE + i * sizeof(Entry), sizeof(Entry));
    if (entry.freeSpace >= requiredSpace) {
      return entry.pageId;
    }
  }
  return INVALID_PAGE_ID;
}
