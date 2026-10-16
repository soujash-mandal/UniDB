#include "CreateTuplePageAction.h"

CreateTuplePageAction::CreateTuplePageAction(TupleNewPagePort &newPagePort,
                                             TupleWritePagePort &writePagePort,
                                             TupleUnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CreateTuplePageActionResult CreateTuplePageAction::execute() {
  TuplePageId tuplePageId = newPagePort.newPage();
  TuplePage page;
  try {
    writePagePort.writePage(tuplePageId, page);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(tuplePageId);
    } catch (...) {
      // Preserve the original page initialization failure.
    }
    throw;
  }
  unpinPagePort.unpinPage(tuplePageId);
  return {tuplePageId, page.getFreeSpace()};
}
