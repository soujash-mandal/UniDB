#include "DeleteTupleAction.h"
#include "../../core/PageHeader.h"
#include "../../core/Slot.h"
#include <cstring>

DeleteTupleAction::DeleteTupleAction(PagePort &pagePort) : pagePort(pagePort) {}

void DeleteTupleAction::execute(const PageId &pageId, SlotId slotId) {
  Page page;
  pagePort.readPage(pageId, page);
  PageHeader header;
  std::memcpy(&header, page.data(), sizeof(PageHeader));
  const std::size_t slotOffset = sizeof(PageHeader) + slotId * sizeof(Slot);
  Slot slot;
  std::memcpy(&slot, page.data() + slotOffset, sizeof(Slot));
  slot.size = 0;
  std::memcpy(page.data() + slotOffset, &slot, sizeof(Slot));
  pagePort.writePage(pageId, page);
}
