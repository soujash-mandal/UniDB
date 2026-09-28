#include "TuplePage.h"

#include <stdexcept>

TuplePage::TuplePage() : header{0} {}

void TuplePage::initialize(TuplePageId pageId) {
  (void)pageId;

  header.slotCount = 0;
  tuples.clear();
}

TupleSlotId TuplePage::insert(const std::vector<char> &tupleData) {
  if (!hasSpace(static_cast<uint16_t>(tupleData.size()))) {
    throw std::runtime_error("TuplePage is full");
  }
  TupleSlotId slotId = header.slotCount;
  tuples.push_back(tupleData);
  header.slotCount++;

  return slotId;
}

std::vector<char> TuplePage::get(TupleSlotId slotId) const {
  if (slotId >= tuples.size()) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  return tuples[slotId];
}

void TuplePage::remove(TupleSlotId slotId) {
  if (slotId >= tuples.size()) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  tuples[slotId].clear();
}

bool TuplePage::hasSpace(uint16_t tupleSize) const {
  std::size_t usedSpace = sizeof(Header);
  for (const auto &tuple : tuples) {
    usedSpace += sizeof(TupleSlotId);
    usedSpace += tuple.size();
  }
  return usedSpace + tupleSize <= PAGE_SIZE;
}
