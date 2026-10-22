#include "TuplePage.h"

#include <cstring>
#include <limits>
#include <stdexcept>

TuplePage::TuplePage() {
  std::memset(bytes, 0, TUPLE_PAGE_SIZE);
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  header->slotCount = TUPLE_PAGE_FORMAT_FLAG;
  header->freeSpaceOffset = TUPLE_PAGE_SIZE;
}

void TuplePage::loadFromBytes(const char *source) {
  std::memcpy(bytes, source, TUPLE_PAGE_SIZE);
  auto *header = reinterpret_cast<TuplePageHeader *>(bytes);
  if ((header->slotCount & TUPLE_PAGE_FORMAT_FLAG) != 0) {
    const uint32_t count = header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
    const std::size_t maxSlots =
        (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
    const std::size_t slotEnd =
        sizeof(TuplePageHeader) +
        static_cast<std::size_t>(count) * sizeof(TupleSlotEntry);
    if (count > maxSlots ||
        header->freeSpaceOffset > TUPLE_PAGE_SIZE ||
        slotEnd > header->freeSpaceOffset) {
      throw std::runtime_error("Invalid tuple page header");
    }
    return;
  }

  // Legacy V1 pages used the same 8-byte header without a format flag.
  // Move live tuples into the current layout while preserving slot IDs.
  const uint32_t legacyCount = header->slotCount;
  const uint32_t legacyFreeSpaceOffset = header->freeSpaceOffset;
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  const std::size_t legacySlotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(legacyCount) * sizeof(TupleSlotEntry);
  if (legacyCount > maxSlots || legacyFreeSpaceOffset > TUPLE_PAGE_SIZE ||
      legacySlotEnd > legacyFreeSpaceOffset) {
    throw std::runtime_error("Invalid legacy tuple page header");
  }

  std::vector<std::vector<char>> tuples(legacyCount);
  std::size_t payloadBytes = 0;
  for (uint32_t i = 0; i < legacyCount; ++i) {
    TupleSlotEntry slot{};
    std::memcpy(&slot,
                bytes + sizeof(TuplePageHeader) +
                    static_cast<std::size_t>(i) * sizeof(TupleSlotEntry),
                sizeof(TupleSlotEntry));
    if (slot.size == 0) {
      // Legacy pages used a nonzero offset for an inserted empty tuple and
      // offset zero for a deleted slot. The current format cannot represent
      // that distinction, so fail explicitly rather than silently losing it.
      if (slot.offset != 0) {
        throw std::runtime_error(
            "Legacy tuple page contains an empty tuple that cannot be migrated "
            "safely");
      }
      continue;
    }
    const bool invalidBounds = slot.offset < legacySlotEnd ||
                               slot.offset < legacyFreeSpaceOffset ||
                               slot.offset > TUPLE_PAGE_SIZE ||
                               slot.size > TUPLE_PAGE_SIZE - slot.offset;
    if (invalidBounds) {
      throw std::runtime_error("Invalid legacy tuple slot bounds");
    }
    tuples[i].assign(bytes + slot.offset, bytes + slot.offset + slot.size);
    payloadBytes += slot.size;
  }
  if (legacySlotEnd + payloadBytes > TUPLE_PAGE_SIZE) {
    throw std::runtime_error(
        "Legacy tuple page cannot fit its tuples");
  }

  std::memset(bytes, 0, TUPLE_PAGE_SIZE);
  header = reinterpret_cast<TuplePageHeader *>(bytes);
  header->slotCount = TUPLE_PAGE_FORMAT_FLAG | legacyCount;
  header->freeSpaceOffset = TUPLE_PAGE_SIZE;
  for (uint32_t i = 0; i < legacyCount; ++i) {
    TupleSlotEntry slot{};
    if (!tuples[i].empty()) {
      const auto tupleSize = static_cast<uint32_t>(tuples[i].size());
      header->freeSpaceOffset -= tupleSize;
      std::memcpy(bytes + header->freeSpaceOffset, tuples[i].data(), tupleSize);
      slot.offset = header->freeSpaceOffset;
      slot.size = tupleSize;
    }
    std::memcpy(bytes + sizeof(TuplePageHeader) +
                    static_cast<std::size_t>(i) * sizeof(TupleSlotEntry),
                &slot, sizeof(TupleSlotEntry));
  }
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
  const TupleSlotId slotId = header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
  const std::size_t slotOffset =
      sizeof(TuplePageHeader) + slotId * sizeof(TupleSlotEntry);
  auto *slot = reinterpret_cast<TupleSlotEntry *>(bytes + slotOffset);
  header->freeSpaceOffset -= tupleSize;
  std::memcpy(bytes + header->freeSpaceOffset, tupleData.data(), tupleSize);
  slot->offset = header->freeSpaceOffset;
  slot->size = tupleSize;
  header->slotCount = TUPLE_PAGE_FORMAT_FLAG | (slotId + 1);
  return slotId;
}

std::vector<char> TuplePage::get(TupleSlotId slotId) {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  if ((header->slotCount & TUPLE_PAGE_FORMAT_FLAG) == 0) {
    throw std::runtime_error("Invalid tuple page format");
  }
  const uint32_t count = header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (count > maxSlots || slotId >= count) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  const auto *slot = reinterpret_cast<const TupleSlotEntry *>(
      bytes + sizeof(TuplePageHeader) +
      static_cast<std::size_t>(slotId) * sizeof(TupleSlotEntry));
  if (slot->size == 0) {
    throw std::runtime_error("Tuple slot is deleted");
  }
  const std::size_t slotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(count) * sizeof(TupleSlotEntry);
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
  if ((header->slotCount & TUPLE_PAGE_FORMAT_FLAG) == 0) {
    throw std::runtime_error("Invalid tuple page format");
  }
  const uint32_t count = header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
  if (slotId >= count) {
    throw std::runtime_error("Invalid TupleSlotId");
  }
  auto *slot = reinterpret_cast<TupleSlotEntry *>(
      bytes + sizeof(TuplePageHeader) +
      static_cast<std::size_t>(slotId) * sizeof(TupleSlotEntry));
  slot->offset = 0;
  slot->size = 0;
}

bool TuplePage::hasSpace(uint32_t tupleSize) {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  if ((header->slotCount & TUPLE_PAGE_FORMAT_FLAG) == 0) {
    return false;
  }
  const uint32_t count = header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (count > maxSlots ||
      header->freeSpaceOffset > TUPLE_PAGE_SIZE) {
    return false;
  }
  const std::size_t nextSlotOffset =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(count) * sizeof(TupleSlotEntry);
  if (nextSlotOffset > header->freeSpaceOffset) {
    return false;
  }
  const std::size_t requiredSpace = sizeof(TupleSlotEntry) + tupleSize;
  return requiredSpace <= header->freeSpaceOffset - nextSlotOffset;
}

uint32_t TuplePage::getFreeSpace() {
  TuplePageHeader header;
  std::memcpy(&header, bytes, sizeof(TuplePageHeader));
  if ((header.slotCount & TUPLE_PAGE_FORMAT_FLAG) == 0) {
    return 0;
  }
  const uint32_t count = header.slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (count > maxSlots ||
      header.freeSpaceOffset > TUPLE_PAGE_SIZE) {
    return 0;
  }
  const std::size_t slotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(count) * sizeof(TupleSlotEntry);
  if (header.freeSpaceOffset < slotEnd) {
    return 0;
  }
  return static_cast<uint32_t>(header.freeSpaceOffset - slotEnd);
}

uint32_t TuplePage::getSlotCount() const {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  if ((header->slotCount & TUPLE_PAGE_FORMAT_FLAG) == 0) {
    throw std::runtime_error("Invalid tuple page format");
  }
  return header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
}

std::vector<std::vector<char>> TuplePage::getAllTuples() const {
  const auto *header = reinterpret_cast<const TuplePageHeader *>(bytes);
  if ((header->slotCount & TUPLE_PAGE_FORMAT_FLAG) == 0) {
    throw std::runtime_error("Invalid tuple page format");
  }
  const uint32_t count = header->slotCount & TUPLE_PAGE_SLOT_COUNT_MASK;
  const std::size_t maxSlots =
      (TUPLE_PAGE_SIZE - sizeof(TuplePageHeader)) / sizeof(TupleSlotEntry);
  if (count > maxSlots ||
      header->freeSpaceOffset > TUPLE_PAGE_SIZE) {
    throw std::runtime_error("Invalid tuple page header");
  }
  const std::size_t slotEnd =
      sizeof(TuplePageHeader) +
      static_cast<std::size_t>(count) * sizeof(TupleSlotEntry);
  if (slotEnd > header->freeSpaceOffset) {
    throw std::runtime_error("Tuple page slots overlap tuple data");
  }

  std::vector<std::vector<char>> tuples;
  for (uint32_t i = 0; i < count; ++i) {
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
