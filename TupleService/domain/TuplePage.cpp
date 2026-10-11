#include "TuplePage.h"

#include <cstring>
#include <limits>
#include <stdexcept>

TuplePage::TuplePage() {
  std::memset(bytes, 0, PAGE_SIZE);
  auto *header = reinterpret_cast<Header *>(bytes);
  header->slotCount = 0;
  header->freeSpaceOffset = PAGE_SIZE;
  header->nextPageId = std::numeric_limits<TuplePageId>::max();
}

TupleSlotId TuplePage::insert(const std::vector<char> tupleData) {
  if (tupleData.empty()) throw std::invalid_argument("Tuple data cannot be empty");
  if (tupleData.size() > PAGE_SIZE || !hasSpace(static_cast<uint32_t>(tupleData.size()))) {
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
  if (slotId >= header->slotCount) throw std::runtime_error("Invalid TupleSlotId");
  const auto *slot = reinterpret_cast<const Slot *>(bytes + sizeof(Header) + slotId * sizeof(Slot));
  if (slot->size == 0) throw std::runtime_error("Tuple slot is deleted");
  return std::vector<char>(bytes + slot->offset, bytes + slot->offset + slot->size);
}

void TuplePage::remove(TupleSlotId slotId) {
  auto *header = reinterpret_cast<Header *>(bytes);
  if (slotId >= header->slotCount) throw std::runtime_error("Invalid TupleSlotId");
  auto *slot = reinterpret_cast<Slot *>(bytes + sizeof(Header) + slotId * sizeof(Slot));
  slot->offset = 0;
  slot->size = 0;
}

bool TuplePage::hasSpace(uint32_t tupleSize) {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  const std::size_t nextSlotOffset = sizeof(Header) + header->slotCount * sizeof(Slot);
  return nextSlotOffset + sizeof(Slot) + tupleSize <= header->freeSpaceOffset;
}

uint32_t TuplePage::getFreeSpace() {
  Header header;
  std::memcpy(&header, bytes, sizeof(Header));
  const uint32_t slotEnd = static_cast<uint32_t>(sizeof(Header) + header.slotCount * sizeof(Slot));
  return header.freeSpaceOffset >= slotEnd ? header.freeSpaceOffset - slotEnd : 0;
}

TuplePageId TuplePage::getNextPageId() const {
  return reinterpret_cast<const Header *>(bytes)->nextPageId;
}
void TuplePage::setNextPageId(TuplePageId pageId) {
  reinterpret_cast<Header *>(bytes)->nextPageId = pageId;
}
uint32_t TuplePage::getSlotCount() const {
  return reinterpret_cast<const Header *>(bytes)->slotCount;
}
std::vector<std::vector<char>> TuplePage::getAllTuples() const {
  const auto *header = reinterpret_cast<const Header *>(bytes);
  std::vector<std::vector<char>> tuples;
  for (uint32_t i = 0; i < header->slotCount; ++i) {
    const auto *slot = reinterpret_cast<const Slot *>(bytes + sizeof(Header) + i * sizeof(Slot));
    if (slot->size != 0) tuples.emplace_back(bytes + slot->offset, bytes + slot->offset + slot->size);
  }
  return tuples;
}
