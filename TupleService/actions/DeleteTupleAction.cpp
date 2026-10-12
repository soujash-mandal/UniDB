#include "DeleteTupleAction.h"

DeleteTupleAction::DeleteTupleAction(FetchPagePort &fetchPagePort,
                                     WritePagePort &writePagePort,
                                     UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void DeleteTupleAction::execute(TuplePageId pageId, TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  try {
    page.remove(slotId);
    writePagePort.writePage(pageId, page);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original operation failure.
    }
    throw;
  }

  unpinPagePort.unpinPage(pageId);
}
