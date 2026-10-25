#include "CreateTupleWithFreeSpaceAction.h"

#include <utility>

CreateTupleWithFreeSpaceAction::CreateTupleWithFreeSpaceAction(
    TupleFetchPagePort &fetchPagePort, TupleWritePagePort &writePagePort,
    TupleUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CreateTupleWithFreeSpaceActionResult CreateTupleWithFreeSpaceAction::execute(
    TuplePageId pageId, std::vector<char> tupleData) {
  TuplePage page = fetchPagePort.fetchPage(pageId);
  TupleSlotId slotId;
  uint32_t remainingFreeSpace;
  try {
    slotId = page.insert(tupleData);
    remainingFreeSpace = page.getFreeSpace();
    writePagePort.writePage(pageId, page);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(pageId);
    } catch (...) {
      // Preserve the original insert or page-write failure.
    }
    throw;
  }

  unpinPagePort.unpinPage(pageId);
  return {slotId, remainingFreeSpace};
}
