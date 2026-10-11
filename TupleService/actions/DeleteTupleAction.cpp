#include "DeleteTupleAction.h"

DeleteTupleAction::DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                                     TupleWritePagePort &writePagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort) {}

void DeleteTupleAction::execute(TuplePageId pageId, TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  page.remove(slotId);
  writePagePort.writePage(pageId, page);
}
