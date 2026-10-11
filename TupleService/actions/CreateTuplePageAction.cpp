#include "CreateTuplePageAction.h"

CreateTuplePageAction::CreateTuplePageAction(TupleNewPagePort &newPagePort,
                                             TupleWritePagePort &writePagePort,
                                             TupleUnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CreateTuplePageActionResult CreateTuplePageAction::execute() {
  TuplePageId tuplePageId = newPagePort.newPage();
  TuplePage page;
  writePagePort.writePage(tuplePageId, page);
  unpinPagePort.unpinPage(tuplePageId);
  return {tuplePageId, page.getFreeSpace()};
}
