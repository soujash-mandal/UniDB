#include "CreateTupleAction.h"

CreateTupleAction::CreateTupleAction(FetchPagePort &fetchPagePort,
                                     UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

TupleSlotId CreateTupleAction::execute(TuplePageId pageId,
                                       std::vector<char> tupleData) {

  TuplePage page = fetchPagePort.fetchPage(pageId);
  TupleSlotId slotId = page.insert(tupleData);
  unpinPagePort.unpinPage(pageId);
  return slotId;
}
