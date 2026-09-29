#include "DeleteTupleAction.h"

DeleteTupleAction::DeleteTupleAction(FetchPagePort &fetchPagePort,
                                     UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

void DeleteTupleAction::execute(TuplePageId pageId, TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  page.remove(slotId);
  unpinPagePort.unpinPage(pageId);
}
