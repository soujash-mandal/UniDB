#include "GetTupleAction.h"

GetTupleAction::GetTupleAction(FetchPagePort &fetchPagePort,
                               UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<char> GetTupleAction::execute(TuplePageId pageId,
                                          TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  std::vector<char> tupleData;
  try {
    tupleData = page.get(slotId);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original operation failure.
    }
    throw;
  }

  unpinPagePort.unpinPage(pageId);
  return tupleData;
}
