#include "GetTuplePageFreeSpaceAction.h"

GetTuplePageFreeSpaceAction::GetTuplePageFreeSpaceAction(
    TupleFetchPagePort &fetchPagePort, TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

uint32_t GetTuplePageFreeSpaceAction::execute(TuplePageId pageId) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  uint32_t freeSpace;
  try {
    freeSpace = page.getFreeSpace();
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original operation failure.
    }
    throw;
  }

  unpinPagePort.unpinPage(pageId);
  return freeSpace;
}
