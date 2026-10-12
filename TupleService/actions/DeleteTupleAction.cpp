#include "DeleteTupleAction.h"

DeleteTupleAction::DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                                     TupleWritePagePort &writePagePort,
                                     TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void DeleteTupleAction::execute(TuplePageId pageId, TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  try {
    page.remove(slotId);
    writePagePort.writePage(pageId, page);
    unpinPagePort.unpinPage(pageId);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original operation failure.
    }
    throw;
  }
}
