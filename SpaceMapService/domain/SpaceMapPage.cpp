#include "SpaceMapPage.h"

#include <cstring>
#include <stdexcept>

SpaceMapPage::SpaceMapPage() {
  std::memset(bytes, 0, PAGE_SIZE);
  Header header;
  header.nextPageId = INVALID_PAGE_ID;
  header.entryCount = 0;
  std::memcpy(bytes, &header, sizeof(Header));
}

SpaceMapPageId SpaceMapPage::getNextPageId() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.nextPageId;
}

void SpaceMapPage::setNextPageId(SpaceMapPageId pageId) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  header.nextPageId = pageId;
  std::memcpy(bytes, &header, sizeof(Header));
}

bool SpaceMapPage::isFull() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  return header.entryCount >= MAX_ENTRIES;
}

void SpaceMapPage::insert(SpaceMapPageId pageId, uint32_t freeSpace) {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  if (header.entryCount >= MAX_ENTRIES) {
    throw std::runtime_error("SpaceMapPage is full");
  }
  Entry entry;
  entry.pageId = pageId;
  entry.freeSpace = freeSpace;
  std::memcpy(bytes + HEADER_SIZE + header.entryCount * sizeof(Entry), &entry,
              sizeof(Entry));
  ++header.entryCount;
  std::memcpy(bytes, &header, sizeof(Header));
}

void SpaceMapPage::update(SpaceMapPageId pageId, uint32_t freeSpace) {
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
  throw std::runtime_error("PageId not found in SpaceMapPage");
}

SpaceMapPageId SpaceMapPage::findPageWithSpace(uint32_t requiredSpace) {
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

std::vector<TuplePageId> SpaceMapPage::getPageIds() const {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  if (header.entryCount > MAX_ENTRIES) {
    throw std::runtime_error("Invalid entry count in SpaceMapPage");
  }

  std::vector<TuplePageId> pageIds;
  pageIds.reserve(header.entryCount);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    Entry entry;
    std::memcpy(&entry, bytes + HEADER_SIZE + i * sizeof(Entry),
                sizeof(Entry));
    if (!isValidPage(entry.pageId)) {
      throw std::runtime_error("Invalid tuple page ID in SpaceMapPage");
    }
    pageIds.push_back(entry.pageId);
  }
  return pageIds;
}
