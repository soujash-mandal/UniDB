#include "TuplePage.h"

#include <cstring>
#include <limits>
#include <stdexcept>

TuplePage::TuplePage() {
  std::memset(bytes, 0, TUPLE_PAGE_SIZE);
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  header->slotCount = 0;
  header->freeSpaceOffset = TUPLE_PAGE_SIZE;
  header->nextPageId = std::numeric_limits<TuplePageId>::max();
}

TupleSlotId TuplePage::insert(const std::vector<char> tupleData) {
  if (tupleData.empty())
    throw std::invalid_argument("Tuple data cannot be empty");
  if (!hasSpace(static_cast<uint32_t>(tupleData.size())))
    throw std::runtime_error("TuplePage is full");
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  TupleSlotId slotId = header->slotCount;
  const std::size_t slotOffset =
      sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry);
  auto *slot = reinterpret_cast<TupleSlotEntry *>(bytes + slotOffset);
  const uint32_t tupleSize = static_cast<uint32_t>(tupleData.size());
  header->freeSpaceOffset -= tupleSize;
  std::memcpy(bytes + header->freeSpaceOffset, tupleData.data(), tupleSize);
  slot->offset = header->freeSpaceOffset;
  slot->size = tupleSize;
  header->slotCount++;
  return slotId;
}

std::vector<char> TuplePage::get(TupleSlotId slotId) {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  if (slotId >= header->slotCount)
    throw std::runtime_error("Invalid TupleSlotId");
  const auto *slot = reinterpret_cast<const TupleSlotEntry *>(
      bytes + sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry));
  if (slot->size == 0)
    throw std::runtime_error("Tuple slot is deleted");
  return std::vector<char>(bytes + slot->offset,
                           bytes + slot->offset + slot->size);
}

void TuplePage::remove(TupleSlotId slotId) {
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  if (slotId >= header->slotCount)
    throw std::runtime_error("Invalid TupleSlotId");
  auto *slot = reinterpret_cast<TupleSlotEntry *>(
      bytes + sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry));
  slot->offset = 0;
  slot->size = 0;
}

bool TuplePage::hasSpace(uint32_t tupleSize) {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  const std::size_t nextSlotOffset =
      sizeof(TuplePageHeader) + header->slotCount * sizeof(TupleSlotEntry);
  return nextSlotOffset + sizeof(TupleSlotEntry) + tupleSize <=
         header->freeSpaceOffset;
}

uint32_t TuplePage::getFreeSpace() {
  TuplePageHeader header;
  std::memcpy(&header, bytes, sizeof(TuplePageHeader));
  const uint32_t slotEnd = static_cast<uint32_t>(
      sizeof(TuplePageHeader) + header.slotCount * sizeof(TupleSlotEntry));
  return header.freeSpaceOffset >= slotEnd ? header.freeSpaceOffset - slotEnd
                                           : 0;
}

TuplePageId TuplePage::getNextPageId() const {
  return reinterpret_cast<const TuplePageHeader *>(bytes)->nextPageId;
}
void TuplePage::setNextPageId(TuplePageId pageId) {
  reinterpret_cast<TuplePageHeader *>(bytes)->nextPageId = pageId;
}
uint32_t TuplePage::getSlotCount() const {
  return reinterpret_cast<const TuplePageHeader *>(bytes)->slotCount;
}
std::vector<std::vector<char>> TuplePage::getAllTuples() const {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  std::vector<std::vector<char>> tuples;
  for (uint32_t i = 0; i < header->slotCount; ++i) {
    const auto *slot = reinterpret_cast<const TupleSlotEntry *>(
        bytes + sizeof(TuplePageHeader) + i * sizeof(TupleSlotEntry));
    if (slot->size != 0)
      tuples.emplace_back(bytes + slot->offset,
                          bytes + slot->offset + slot->size);
  }
  return tuples;
}

std::vector<TupleRecord> TuplePage::getAllTupleRecords() const {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  std::vector<TupleRecord> records;
  for (uint32_t i = 0; i < header->slotCount; ++i) {
    const auto *slot = reinterpret_cast<const TupleSlotEntry *>(
        bytes + sizeof(TuplePageHeader) + i * sizeof(TupleSlotEntry));
    if (slot->size != 0) {
      records.push_back({i, std::vector<char>(bytes + slot->offset,
                                              bytes + slot->offset + slot->size)});
    }
  }
  return records;
}
