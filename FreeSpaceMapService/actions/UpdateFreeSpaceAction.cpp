#include "UpdateFreeSpaceAction.h"

UpdateFreeSpaceAction::UpdateFreeSpaceAction(FetchPagePort &fetchPagePort,
                                             WritePagePort &writePagePort,
                                             UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void UpdateFreeSpaceAction::execute(FSMPageId fsmPageId, TuplePageId pageId,
                                    uint32_t updatedfFreeSpace) {
  FSMPage fsmPage = fetchPagePort.fetchPage(fsmPageId);
  fsmPage.update(pageId, updatedfFreeSpace);
  writePagePort.writePage(fsmPageId, fsmPage);
  unpinPagePort.unpinPage(fsmPageId);
  return;
}
