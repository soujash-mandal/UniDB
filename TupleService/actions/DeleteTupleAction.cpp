#include "DeleteTupleAction.h"

#include <cstring>

#include "../../core/Slot.h"
#include "../domain/PageHeader.h"
#include "../domain/TupleNotFoundException.h"

DeleteTupleAction::DeleteTupleAction(FetchPagePort &fetchPagePort,
                                     UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

void DeleteTupleAction::execute(const PageId &pageId, SlotId slotId) {
  Page &page = fetchPagePort.fetchPage(pageId);

  PageHeader header;
  std::memcpy(&header, page.data(), sizeof(PageHeader));
  if (slotId >= header.slotCount) {
    unpinPagePort.unpinPage(pageId, false);
    throw TupleNotFoundException("Tuple does not exist");
  }
  const std::size_t slotOffset = sizeof(PageHeader) + slotId * sizeof(Slot);
  Slot slot;
  std::memcpy(&slot, page.data() + slotOffset, sizeof(Slot));
  if (slot.size == 0) {
    unpinPagePort.unpinPage(pageId, false);
    throw TupleNotFoundException("Tuple has already been deleted");
  }

  slot.size = 0;
  std::memcpy(page.data() + slotOffset, &slot, sizeof(Slot));
  unpinPagePort.unpinPage(pageId, true);
}
