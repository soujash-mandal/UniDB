#include "CreateTuplePageAction.h"

CreateTuplePageAction::CreateTuplePageAction(NewPagePort &newPagePort,
                                             WritePagePort &writePagePort,
                                             UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

TuplePageId CreateTuplePageAction::execute() {
  TuplePageId tuplePageId = newPagePort.newPage();
  TuplePage page;
  writePagePort.writePage(tuplePageId, page);
  unpinPagePort.unpinPage(tuplePageId);
  return tuplePageId;
}
