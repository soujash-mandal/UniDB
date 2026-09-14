#include "GetTupleAction.h"

#include <cstring>

#include "../../core/PageHeader.h"
#include "../../core/Slot.h"

GetTupleAction::GetTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void GetTupleAction::execute(const PageId &pageId, uint16_t slotId,
                             char *tupleData) {

  Page page;
  pagePort.readPage(pageId, page);
  PageHeader header;
  std::memcpy(&header, page.data(), sizeof(PageHeader));
  const std::size_t slotOffset = sizeof(PageHeader) + slotId * sizeof(Slot);
  Slot slot;
  std::memcpy(&slot, page.data() + slotOffset, sizeof(Slot));
  std::memcpy(tupleData, page.data() + slot.offset, slot.size);
}
