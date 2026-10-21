#include "CreateTupleAction.h"

CreateTupleAction::CreateTupleAction(TupleFetchPagePort &fetchPagePort,
                                     TupleWritePagePort &writePagePort,
                                     TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

TupleSlotId CreateTupleAction::execute(TuplePageId pageId,
                                       std::vector<char> tupleData) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  TupleSlotId slotId;
  try {
    slotId = page.insert(tupleData);
    writePagePort.writePage(pageId, page);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original operation failure.
    }
    throw;
  }

  unpinPagePort.unpinPage(pageId);
  return slotId;
}
