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
  if (tupleData.empty()) {
    throw std::invalid_argument("Tuple data cannot be empty");
  }
  if (tupleData.size() > std::numeric_limits<uint32_t>::max()) {
    throw std::length_error("Tuple exceeds the V1 tuple size limit");
  }
  const auto tupleSize = static_cast<uint32_t>(tupleData.size());
  if (!hasSpace(tupleSize)) {
    throw std::runtime_error("TuplePage is full");
  }
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  const TupleSlotId slotId = header->slotCount;
  const std::size_t slotOffset =
      sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry);
  auto *slot = reinterpret_cast<TupleSlotEntry *>(bytes + slotOffset);
  header->freeSpaceOffset -= tupleSize;
  std::memcpy(bytes + header->freeSpaceOffset, tupleData.data(), tupleSize);
  slot->offset = header->freeSpaceOffset;
  slot->size = tupleSize;
  header->slotCount++;
  return slotId;
}

std::vector<char> TuplePage::get(TupleSlotId slotId) {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (header->slotCount > maxSlots || slotId >= header->slotCount) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  const auto *slot = reinterpret_cast<const TupleSlotEntry *>(
      bytes + sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry));
  if (slot->size == 0) {
    throw std::runtime_error("Tuple slot is deleted");
  }
  const std::size_t slotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(header->slotCount) * sizeof(TupleSlotEntry);
  const bool invalidBounds =
      slot->offset < slotEnd || slot->offset < header->freeSpaceOffset ||
      slot->offset > TUPLE_PAGE_SIZE ||
      slot->size > TUPLE_PAGE_SIZE - slot->offset;
  if (invalidBounds) {
    throw std::runtime_error("Invalid tuple slot bounds");
  }
  return std::vector<char>(bytes + slot->offset,
                           bytes + slot->offset + slot->size);
}

void TuplePage::remove(TupleSlotId slotId) {
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  if (slotId >= header->slotCount) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  auto *slot = reinterpret_cast<TupleSlotEntry *>(
      bytes + sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry));
  slot->offset = 0;
  slot->size = 0;
}

bool TuplePage::hasSpace(uint32_t tupleSize) {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (header->slotCount > maxSlots ||
      header->freeSpaceOffset > TUPLE_PAGE_SIZE) {
    return false;
  }
  const std::size_t nextSlotOffset =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(header->slotCount) * sizeof(TupleSlotEntry);
  if (nextSlotOffset > header->freeSpaceOffset) {
    return false;
  }
  const std::size_t requiredSpace = sizeof(TupleSlotEntry) + tupleSize;
  return requiredSpace <= header->freeSpaceOffset - nextSlotOffset;
}

uint32_t TuplePage::getFreeSpace() {
  TuplePageHeader header;
  std::memcpy(&header, bytes, sizeof(TuplePageHeader));
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (header.slotCount > maxSlots ||
      header.freeSpaceOffset > TUPLE_PAGE_SIZE) {
    return 0;
  }
  const std::size_t slotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(header.slotCount) * sizeof(TupleSlotEntry);
  if (header.freeSpaceOffset < slotEnd) {
    return 0;
  }
  return static_cast<uint32_t>(header.freeSpaceOffset - slotEnd);
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
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (header->slotCount > maxSlots ||
      header->freeSpaceOffset > TUPLE_PAGE_SIZE) {
    throw std::runtime_error("Invalid tuple page header");
  }
  const std::size_t slotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(header->slotCount) * sizeof(TupleSlotEntry);
  if (slotEnd > header->freeSpaceOffset) {
    throw std::runtime_error("Tuple page slots overlap tuple data");
  }

  std::vector<std::vector<char>> tuples;
  for (uint32_t i = 0; i < header->slotCount; ++i) {
    const auto *slot = reinterpret_cast<const TupleSlotEntry *>(
        bytes + sizeof(TuplePageHeader) +
        static_cast<std::size_t>(i) * sizeof(TupleSlotEntry));
    if (slot->size == 0) {
      continue;
    }
    const bool invalidBounds =
        slot->offset < slotEnd || slot->offset < header->freeSpaceOffset ||
        slot->offset > TUPLE_PAGE_SIZE ||
        slot->size > TUPLE_PAGE_SIZE - slot->offset;
    if (invalidBounds) {
      throw std::runtime_error("Invalid tuple slot bounds");
    }
    tuples.emplace_back(bytes + slot->offset,
                        bytes + slot->offset + slot->size);
  }
  return tuples;
}
