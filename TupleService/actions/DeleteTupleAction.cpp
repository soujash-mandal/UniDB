#include "DeleteTupleAction.h"

DeleteTupleAction::DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                                     TupleWritePagePort &writePagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort) {}

DeleteTupleAction::DeleteTupleAction(TupleFetchPagePort &fetchPagePort,
                                     TupleWritePagePort &writePagePort,
                                     TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(&unpinPagePort) {}

void DeleteTupleAction::execute(TuplePageId pageId, TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  page.remove(slotId);
  writePagePort.writePage(pageId, page);
  if (unpinPagePort != nullptr)
    unpinPagePort->unpinPage(pageId);
}
