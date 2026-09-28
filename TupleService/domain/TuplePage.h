#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

using TuplePageId = uint32_t;
using TupleSlotId = uint16_t;

class TuplePage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;
  TuplePage();
  void initialize(TuplePageId pageId);
  TupleSlotId insert(const std::vector<char> &tupleData);
  std::vector<char> get(TupleSlotId slotId) const;
  void remove(TupleSlotId slotId);
  bool hasSpace(uint16_t tupleSize) const;

private:
  struct Header {
    uint16_t slotCount;
  };

  Header header;
  std::vector<std::vector<char>> tuples;
};
