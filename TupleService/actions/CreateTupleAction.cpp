#include "CreateTupleAction.h"

#include <cstdint>
#include <cstring>

#include "../../core/PageHeader.h"
#include "../../core/Slot.h"
#include "../domain/PageFullException.h"

CreateTupleAction::CreateTupleAction(FetchPagePort &fetchPagePort,
                                     UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

uint16_t CreateTupleAction::execute(const PageId &pageId, const char *tupleData,
                                    uint16_t tupleSize) {

  Page &page = fetchPagePort.fetchPage(pageId);
  auto *header = reinterpret_cast<PageHeader *>(page.data());
  uint16_t slotCount = header->slotCount;
  uint16_t freeSpaceOffset = header->freeSpaceOffset;

  // 1. Calculate free space.
  uint16_t freeSpaceStart = sizeof(PageHeader) + slotCount * sizeof(Slot);
  uint16_t freeSpaceEnd = freeSpaceOffset;
  uint16_t freeSpace = freeSpaceEnd - freeSpaceStart;

  // 2. Check space availability.
  if (tupleSize + sizeof(Slot) > freeSpace) {
    unpinPagePort.unpinPage(pageId, false);
    throw PageFullException("Not enough space in page");
  }

  // 3. Write tuple at the end of the free space.
  uint16_t newTupleOffset = freeSpaceEnd - tupleSize;
  std::memcpy(page.data() + newTupleOffset, tupleData, tupleSize);

  // 4. Write new slot.
  Slot slot;
  slot.offset = newTupleOffset;
  slot.size = tupleSize;
  std::memcpy(page.data() + sizeof(PageHeader) + slotCount * sizeof(Slot),
              &slot, sizeof(Slot));

  // 5. The new tuple's slot ID is the previous slot count.
  uint16_t latestTupleSlotId = slotCount;

  // 6. Update page header.
  header->slotCount++;
  header->freeSpaceOffset = newTupleOffset;

  // 7. Page was modified.
  unpinPagePort.unpinPage(pageId, true);

  return latestTupleSlotId;
}
