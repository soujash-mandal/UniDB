#include "CreatePageAction.h"

CreatePageAction::CreatePageAction(NewPagePort &newPagePort,
                                   WritePagePort &writePagePort,
                                   UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

TuplePageId CreatePageAction::execute() {
  TuplePageId tuplePageId = newPagePort.newPage();
  TuplePage page;
  writePagePort.writePage(tuplePageId, page);
  unpinPagePort.unpinPage(tuplePageId);
  return tuplePageId;
}
