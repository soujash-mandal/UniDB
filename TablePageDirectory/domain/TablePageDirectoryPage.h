#pragma once

#include <cstddef>
#include <cstdint>

using TablePageDirectoryPageId = uint32_t;
using TuplePageId = uint32_t;

static constexpr TablePageDirectoryPageId TABLE_PAGE_DIRECTORY_INVALID_PAGE_ID =
    UINT32_MAX;

class TablePageDirectoryPage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;

  TablePageDirectoryPage();
  char *data() { return bytes; }
  const char *data() const { return bytes; }

  TablePageDirectoryPageId getNextPageId() const;
  void setNextPageId(TablePageDirectoryPageId pageId);

  bool isFull() const;
  uint32_t size() const;
  bool contains(TuplePageId pageId) const;
  uint32_t getFreeSpace(TuplePageId pageId) const;
  TuplePageId findPageWithSpace(uint32_t requiredSpace) const;

  void insert(TuplePageId pageId, uint32_t freeSpace);
  void update(TuplePageId pageId, uint32_t freeSpace);

private:
  char bytes[PAGE_SIZE]{};
};
