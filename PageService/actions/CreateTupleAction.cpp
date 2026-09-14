#include "CreateTupleAction.h"

#include <cstdint>
#include <cstring>

#include "../../core/PageHeader.h"
#include "../../core/Slot.h"
#include "../domain/PageFullException.h"

CreateTupleAction::CreateTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

uint16_t CreateTupleAction::execute(const PageId &pageId, const char *tupleData,
                                    uint16_t tupleSize) {
  Page page;
  pagePort.readPage(pageId, page);
  auto *header = reinterpret_cast<PageHeader *>(page.data());
  uint16_t slotCount = header->slotCount;
  uint16_t freeSpaceOffset = header->freeSpaceOffset;

  // free space Calculation
  uint16_t freeSpaceStart = sizeof(PageHeader) + slotCount * sizeof(Slot);
  uint16_t freeSpaceEnd = freeSpaceOffset;
  uint16_t freeSpace = freeSpaceEnd - freeSpaceStart;

  // check space availability
  if (tupleSize + sizeof(Slot) > freeSpace)
    throw PageFullException("Not enough space in page");

  // write new tuple to page
  uint16_t newTupleOffset = freeSpaceEnd - tupleSize;
  std::memcpy(page.data() + newTupleOffset, tupleData, tupleSize);

  // write new slot to slot map
  Slot slot;
  slot.offset = newTupleOffset;
  slot.size = tupleSize;
  std::memcpy(page.data() + sizeof(PageHeader) + slotCount * sizeof(Slot),
              &slot, sizeof(Slot));

  // slotId to return
  uint16_t latestTupleSlotId = slotCount;

  // update header
  header->slotCount++;
  header->freeSpaceOffset = newTupleOffset;
  pagePort.writePage(pageId, page);

  return latestTupleSlotId;
}
