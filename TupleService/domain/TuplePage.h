#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>


using TuplePageId = uint32_t;
using TupleSlotId = uint32_t;

namespace {
static constexpr std::size_t TUPLE_PAGE_SIZE = 8192;
static constexpr uint32_t TUPLE_PAGE_FORMAT_FLAG = 0x80000000U;
static constexpr uint32_t TUPLE_PAGE_SLOT_COUNT_MASK = 0x7FFFFFFFU;
static constexpr std::size_t TUPLE_PAGE_NEXT_PAGE_ID_OFFSET =
    TUPLE_PAGE_SIZE - sizeof(TuplePageId);

struct TuplePageHeader {
  uint32_t slotCount;
  uint32_t freeSpaceOffset;
};
struct TupleSlotEntry {
  uint32_t offset;
  uint32_t size;
};
} // namespace

class TuplePage {
public:
  TuplePage();
  char *data() { return bytes; }
  const char *data() const { return bytes; }
  void loadFromBytes(const char *source);
  TupleSlotId insert(const std::vector<char> tupleData);
  std::vector<char> get(TupleSlotId slotId);
  void remove(TupleSlotId slotId);
  bool hasSpace(uint32_t tupleSize);
  uint32_t getFreeSpace();
  TuplePageId getNextPageId() const;
  void setNextPageId(TuplePageId pageId);
  uint32_t getSlotCount() const;
  std::vector<std::vector<char>> getAllTuples() const;

private:
  alignas(std::max_align_t) char bytes[TUPLE_PAGE_SIZE]{};
};
