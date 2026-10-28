#include "SpaceMapPage.h"

#include <cstring>
#include <stdexcept>

SpaceMapPage::SpaceMapPage() {
  std::memset(bytes, 0, SPACE_MAP_PAGE_SIZE);
  SpaceMapHeader header;
  header.nextPageId = SPACE_MAP_INVALID_PAGE_ID;
  header.entryCount = 0;
  std::memcpy(bytes, &header, sizeof(SpaceMapHeader));
}

SpaceMapPageId SpaceMapPage::getNextPageId() {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  return header.nextPageId;
}

void SpaceMapPage::setNextPageId(SpaceMapPageId pageId) {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  header.nextPageId = pageId;
  std::memcpy(bytes, &header, sizeof(SpaceMapHeader));
}

bool SpaceMapPage::isFull() {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  return header.entryCount >= SPACE_MAP_MAX_ENTRIES;
}

void SpaceMapPage::insert(SpaceMapPageId pageId, uint32_t freeSpace) {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  if (header.entryCount >= SPACE_MAP_MAX_ENTRIES) {
    throw std::runtime_error("SpaceMapPage is full");
  }
  SpaceMapEntry entry;
  entry.pageId = pageId;
  entry.freeSpace = freeSpace;
  std::memcpy(bytes + SPACE_MAP_HEADER_SIZE +
                  header.entryCount * sizeof(SpaceMapEntry),
              &entry, sizeof(SpaceMapEntry));
  ++header.entryCount;
  std::memcpy(bytes, &header, sizeof(SpaceMapHeader));
}

void SpaceMapPage::update(SpaceMapPageId pageId, uint32_t freeSpace) {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    SpaceMapEntry entry;
    std::memcpy(&entry,
                bytes + SPACE_MAP_HEADER_SIZE + i * sizeof(SpaceMapEntry),
                sizeof(SpaceMapEntry));
    if (entry.pageId == pageId) {
      entry.freeSpace = freeSpace;
      std::memcpy(bytes + SPACE_MAP_HEADER_SIZE + i * sizeof(SpaceMapEntry),
                  &entry, sizeof(SpaceMapEntry));
      return;
    }
  }
  throw std::runtime_error("PageId not found in SpaceMapPage");
}

SpaceMapPageId SpaceMapPage::findPageWithSpace(uint32_t requiredSpace) {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    SpaceMapEntry entry;
    std::memcpy(&entry,
                bytes + SPACE_MAP_HEADER_SIZE + i * sizeof(SpaceMapEntry),
                sizeof(SpaceMapEntry));
    if (entry.freeSpace >= requiredSpace) {
      return entry.pageId;
    }
  }
  return SPACE_MAP_INVALID_PAGE_ID;
}

std::vector<TuplePageId> SpaceMapPage::getPageIds() const {
  SpaceMapHeader header;
  std::memcpy(&header, bytes, sizeof(SpaceMapHeader));
  if (header.entryCount > SPACE_MAP_MAX_ENTRIES) {
    throw std::runtime_error("Invalid entry count in SpaceMapPage");
  }

  std::vector<TuplePageId> pageIds;
  pageIds.reserve(header.entryCount);
  for (uint32_t i = 0; i < header.entryCount; ++i) {
    SpaceMapEntry entry;
    std::memcpy(&entry,
                bytes + SPACE_MAP_HEADER_SIZE + i * sizeof(SpaceMapEntry),
                sizeof(SpaceMapEntry));
    if (!isValidPage(entry.pageId)) {
      throw std::runtime_error("Invalid tuple page ID in SpaceMapPage");
    }
    pageIds.push_back(entry.pageId);
  }
  return pageIds;
}
