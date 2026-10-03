#pragma once

#include <cstddef>
#include <cstdint>

using FSMPageId = uint32_t;
using TableId = uint32_t;

namespace {
static constexpr std::size_t PAGE_SIZE = 8192;
static constexpr FSMPageId INVALID_PAGE_ID = UINT32_MAX;
struct Header {
  FSMPageId nextPageId;
  uint32_t entryCount;
};
struct Entry {
  FSMPageId pageId;
  uint32_t freeSpace;
};
static constexpr uint32_t HEADER_SIZE = sizeof(Header);
static constexpr uint32_t MAX_ENTRIES =
    (PAGE_SIZE - HEADER_SIZE) / sizeof(Entry);
} // namespace

class FSMPage {
public:
  FSMPage();

  FSMPageId getNextPageId();
  void setNextPageId(FSMPageId pageId);

  bool isFull();

  void addEntry(FSMPageId pageId, uint32_t freeSpace);
  void updateEntry(FSMPageId pageId, uint32_t freeSpace);
  FSMPageId findPageWithSpace(uint32_t requiredSpace);

  char *data();
  const char *data() const;

private:
  char bytes[PAGE_SIZE]{};
  uint32_t getEntryCount();
};
