#include "CreateTupleAction.h"

CreateTupleAction::CreateTupleAction(TupleFetchPagePort &fetchPagePort,
                                     TupleWritePagePort &writePagePort,
                                     TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

TupleSlotId CreateTupleAction::execute(TuplePageId pageId,
                                      std::vector<char> tupleData) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  try {
    const TupleSlotId slotId = page.insert(tupleData);
    writePagePort.writePage(pageId, page);
    unpinPagePort.unpinPage(pageId);
    return slotId;
  } catch (...) {
    try { unpinPagePort.unpinPage(pageId); } catch (...) {}
    throw;
  }
}
