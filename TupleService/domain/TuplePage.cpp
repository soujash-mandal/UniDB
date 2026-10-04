#include "TuplePage.h"

#include <cstring>
#include <stdexcept>

TuplePage::TuplePage() {
  std::memset(bytes, 0, PAGE_SIZE);
  auto *header = reinterpret_cast<Header *>(bytes);
  header->slotCount = 0;
  header->freeSpaceOffset = PAGE_SIZE;
}

TupleSlotId TuplePage::insert(const std::vector<char> tupleData) {
  if (!hasSpace(static_cast<uint32_t>(tupleData.size()))) {
    throw std::runtime_error("TuplePage is full");
  }
  auto *header = reinterpret_cast<Header *>(bytes);
  TupleSlotId slotId = header->slotCount;
  std::size_t slotOffset = sizeof(Header) + slotId * sizeof(Slot);
  auto *slot = reinterpret_cast<Slot *>(bytes + slotOffset);
  uint32_t tupleSize = static_cast<uint32_t>(tupleData.size());
  header->freeSpaceOffset -= tupleSize;
  std::memcpy(bytes + header->freeSpaceOffset, tupleData.data(), tupleSize);
  slot->offset = header->freeSpaceOffset;
  slot->size = tupleSize;
  header->slotCount++;
  return slotId;
}

std::vector<char> TuplePage::get(TupleSlotId slotId) {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  if (slotId >= header->slotCount) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  std::size_t slotOffset = sizeof(Header) + slotId * sizeof(Slot);
  const auto *slot = reinterpret_cast<const Slot *>(bytes + slotOffset);
  return std::vector<char>(bytes + slot->offset,
                           bytes + slot->offset + slot->size);
}

void TuplePage::remove(TupleSlotId slotId) {
  auto *header = reinterpret_cast<Header *>(bytes);
  if (slotId >= header->slotCount) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  auto *slot =
      reinterpret_cast<Slot *>(bytes + sizeof(Header) + slotId * sizeof(Slot));
  slot->offset = 0;
  slot->size = 0;
}

bool TuplePage::hasSpace(uint32_t tupleSize) {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  std::size_t nextSlotOffset =
      sizeof(Header) + header->slotCount * sizeof(Slot);
  return nextSlotOffset + sizeof(Slot) + tupleSize <= header->freeSpaceOffset;
}

uint32_t TuplePage::getFreeSpace() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  uint32_t slotEnd = sizeof(Header) + header.slotCount * sizeof(Slot);
  return header.freeSpaceOffset - slotEnd;
}
