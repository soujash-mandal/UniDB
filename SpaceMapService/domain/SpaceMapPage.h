#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

using SpaceMapPageId = uint32_t;
using TuplePageId = uint32_t;

namespace {
static constexpr std::size_t SPACE_MAP_PAGE_SIZE = 8192;
static constexpr SpaceMapPageId SPACE_MAP_INVALID_PAGE_ID = UINT32_MAX;
struct SpaceMapHeader {
  SpaceMapPageId nextPageId;
  uint32_t entryCount;
};
struct SpaceMapEntry {
  SpaceMapPageId pageId;
  uint32_t freeSpace;
};
static constexpr uint32_t SPACE_MAP_HEADER_SIZE = sizeof(SpaceMapHeader);
static constexpr uint32_t SPACE_MAP_MAX_ENTRIES =
    (SPACE_MAP_PAGE_SIZE - SPACE_MAP_HEADER_SIZE) / sizeof(SpaceMapEntry);
} // namespace

class SpaceMapPage {
public:
  SpaceMapPage();
  char *data() { return bytes; }
  const char *data() const { return bytes; }

  static bool isValidPage(uint32_t pageId) {
    return !(pageId == SPACE_MAP_INVALID_PAGE_ID);
  }

  SpaceMapPageId getNextPageId();
  void setNextPageId(SpaceMapPageId pageId);

  SpaceMapPageId findPageWithSpace(uint32_t requiredSpace);
  std::vector<TuplePageId> getPageIds() const;
  void insert(SpaceMapPageId pageId, uint32_t freeSpace);
  void update(SpaceMapPageId pageId, uint32_t freeSpace);
  bool isFull();

private:
  char bytes[SPACE_MAP_PAGE_SIZE]{};
};
