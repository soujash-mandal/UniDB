#include "CreateTupleAction.h"

CreateTupleAction::CreateTupleAction(FetchPagePort &fetchPagePort,
                                     WritePagePort &writePagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort) {}

TupleSlotId CreateTupleAction::execute(TuplePageId pageId,
                                       std::vector<char> tupleData) {

  TuplePage page = fetchPagePort.fetchPage(pageId);
  TupleSlotId slotId = page.insert(tupleData);
  writePagePort.writePage(pageId, page);
  return slotId;
}
