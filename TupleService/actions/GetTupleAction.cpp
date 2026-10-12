#include "GetTupleAction.h"

GetTupleAction::GetTupleAction(TupleFetchPagePort &fetchPagePort,
                               TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<char> GetTupleAction::execute(TuplePageId pageId,
                                          TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  try {
    std::vector<char> tupleData = page.get(slotId);
    unpinPagePort.unpinPage(pageId);
    return tupleData;
  } catch (...) {
    try { unpinPagePort.unpinPage(pageId); } catch (...) {}
    throw;
  }
}
