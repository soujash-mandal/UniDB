#pragma once

#include <cstddef>
#include <cstdint>\n#include <vector>

using SpaceMapPageId = uint32_t;
using TuplePageId = uint32_t;

namespace {
static constexpr std::size_t PAGE_SIZE = 8192;
static constexpr SpaceMapPageId INVALID_PAGE_ID = UINT32_MAX;
struct Header {
  SpaceMapPageId nextPageId;
  uint32_t entryCount;
};
struct Entry {
  SpaceMapPageId pageId;
  uint32_t freeSpace;
};
static constexpr uint32_t HEADER_SIZE = sizeof(Header);
static constexpr uint32_t MAX_ENTRIES =
    (PAGE_SIZE - HEADER_SIZE) / sizeof(Entry);
} // namespace

class SpaceMapPage {
public:
  SpaceMapPage();
  char *data() { return bytes; }
  const char *data() const { return bytes; }

  static bool isValidPage(uint32_t pageId) {
    return !(pageId == INVALID_PAGE_ID);
  }

  SpaceMapPageId getNextPageId();
  void setNextPageId(SpaceMapPageId pageId);

  SpaceMapPageId findPageWithSpace(uint32_t requiredSpace);\n  std::vector<TuplePageId> getPageIds() const;
  void insert(SpaceMapPageId pageId, uint32_t freeSpace);
  void update(SpaceMapPageId pageId, uint32_t freeSpace);
  bool isFull();

private:
  char bytes[PAGE_SIZE]{};
};
