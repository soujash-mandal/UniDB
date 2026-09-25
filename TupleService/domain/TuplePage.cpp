#include "TuplePage.h"

#include <cstring>
#include <stdexcept>

namespace {
struct TuplePageHeader {
  uint16_t slotCount;
  uint16_t freeSpaceOffset;
};

struct TupleSlot {
  uint16_t offset;
  uint16_t size;
};

TuplePageHeader readHeader(const Page &page) {
  TuplePageHeader header{};
  std::memcpy(&header, page.data(), sizeof(TuplePageHeader));
  return header;
}

void writeHeader(Page &page, const TuplePageHeader &header) {
  std::memcpy(page.data(), &header, sizeof(TuplePageHeader));
}

TupleSlot readSlot(const Page &page, SlotId slotId) {
  TupleSlot slot{};
  const std::size_t slotOffset =
      sizeof(TuplePageHeader) + slotId * sizeof(TupleSlot);
  std::memcpy(&slot, page.data() + slotOffset, sizeof(TupleSlot));
  return slot;
}

void writeSlot(Page &page, SlotId slotId, const TupleSlot &slot) {
  const std::size_t slotOffset =
      sizeof(TuplePageHeader) + slotId * sizeof(TupleSlot);
  std::memcpy(page.data() + slotOffset, &slot, sizeof(TupleSlot));
}

uint16_t calculateFreeSpace(const TuplePageHeader &header) {
  const uint16_t freeSpaceStart =
      sizeof(TuplePageHeader) + header.slotCount * sizeof(TupleSlot);
  return header.freeSpaceOffset - freeSpaceStart;
}
} // namespace

TuplePage::TuplePage(Page &page) : page(page) {}
void TuplePage::initialize(Page &page, PageId pageId) {
  TuplePageHeader header{};
  header.slotCount = 0;
  header.freeSpaceOffset = Page::PAGE_SIZE;
  writeHeader(page, header);
}

SlotId TuplePage::insert(const char *tupleData, uint16_t tupleSize) {
  TuplePageHeader header = readHeader(page);
  const uint16_t freeSpace = calculateFreeSpace(header);
  if (tupleSize + sizeof(TupleSlot) > freeSpace) {
    throw std::runtime_error("Not enough space in tuple page");
  }
  const SlotId newSlotId = header.slotCount;
  const uint16_t tupleOffset = header.freeSpaceOffset - tupleSize;
  std::memcpy(page.data() + tupleOffset, tupleData, tupleSize);
  TupleSlot slot{};
  slot.offset = tupleOffset;
  slot.size = tupleSize;
  writeSlot(page, newSlotId, slot);
  header.slotCount++;
  header.freeSpaceOffset = tupleOffset;
  writeHeader(page, header);
  return newSlotId;
}

void TuplePage::get(SlotId slotId, char *tupleData) const {
  TuplePageHeader header = readHeader(page);
  if (slotId >= header.slotCount) {
    throw std::runtime_error("slotId is greater than slotCount");
  }
  TupleSlot slot = readSlot(page, slotId);
  if (slot.size == 0) {
    throw std::runtime_error("Tuple has been deleted");
  }
  std::memcpy(tupleData, page.data() + slot.offset, slot.size);
}

void TuplePage::remove(SlotId slotId) {
  TuplePageHeader header = readHeader(page);
  if (slotId >= header.slotCount) {
    throw std::runtime_error("slotId is greater than slotCount");
  }
  TupleSlot slot = readSlot(page, slotId);
  if (slot.size == 0) {
    throw std::runtime_error("Tuple has already been deleted");
  }
  slot.size = 0;
  writeSlot(page, slotId, slot);
}

bool TuplePage::getAvailableSpace() const {
  TuplePageHeader header = readHeader(page);
  return calculateFreeSpace(header) > sizeof(TupleSlot);
}