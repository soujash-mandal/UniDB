#include "GetTupleAction.h"

#include <cstring>

#include "../../core/PageHeader.h"
#include "../../core/Slot.h"
#include "../domain/TupleNotFoundException.h"

GetTupleAction::GetTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void GetTupleAction::execute(const PageId &pageId, uint16_t slotId,
                             char *tupleData) {

  Page page;
  pagePort.readPage(pageId, page);
  PageHeader header;
  std::memcpy(&header, page.data(), sizeof(PageHeader));

  if (slotId >= header.slotCount) {
    throw TupleNotFoundException("Tuple does not exist");
  }
  const std::size_t slotOffset = sizeof(PageHeader) + slotId * sizeof(Slot);
  Slot slot;
  std::memcpy(&slot, page.data() + slotOffset, sizeof(Slot));

  if (slot.size == 0) {
    throw TupleNotFoundException("Tuple has been deleted");
  }

  std::memcpy(tupleData, page.data() + slot.offset, slot.size);
}
