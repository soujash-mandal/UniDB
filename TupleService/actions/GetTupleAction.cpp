#include "GetTupleAction.h"

GetTupleAction::GetTupleAction(FetchPagePort &fetchPagePort,
                               UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

std::vector<char> GetTupleAction::execute(TuplePageId pageId,
                                          TupleSlotId slotId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  std::vector<char> tupleData = page.get(slotId);
  unpinPagePort.unpinPage(pageId);
  return tupleData;
}
