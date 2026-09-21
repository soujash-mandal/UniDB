#include "GetTupleAction.h"

#include <cstring>

#include "../../core/PageHeader.h"
#include "../../core/Slot.h"
#include "../domain/TupleNotFoundException.h"

GetTupleAction::GetTupleAction(FetchPagePort &fetchPagePort,
                               UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

void GetTupleAction::execute(const PageId &pageId, uint16_t slotId,
                             char *tupleData) {
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
    throw TupleNotFoundException("Tuple has been deleted");
  }

  std::memcpy(tupleData, page.data() + slot.offset, slot.size);
  unpinPagePort.unpinPage(pageId, false);
}
