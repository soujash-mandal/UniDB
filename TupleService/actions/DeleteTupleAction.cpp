#include "DeleteTupleAction.h"

DeleteTupleAction::DeleteTupleAction(FetchPagePort &fetchPagePort,
                                     WritePagePort &writePagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort) {}

void DeleteTupleAction::execute(TuplePageId pageId, TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  page.remove(slotId);
  writePagePort.writePage(pageId, page);
}
