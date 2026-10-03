#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

using TuplePageId = uint32_t;
using TupleSlotId = uint32_t;

namespace {
static constexpr std::size_t PAGE_SIZE = 8192;
struct Header {
  uint32_t slotCount;
  uint32_t freeSpaceOffset;
};

struct Slot {
  uint32_t offset;
  uint32_t size;
};
} // namespace

class TuplePage {
public:
  TuplePage();
  char *data() { return bytes; }
  const char *data() const { return bytes; }

  TupleSlotId insert(const std::vector<char> tupleData);
  std::vector<char> get(TupleSlotId slotId);
  void remove(TupleSlotId slotId);
  bool hasSpace(uint32_t tupleSize);

private:
  char bytes[PAGE_SIZE]{};
};
