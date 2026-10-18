#include "UpdateFreeSpaceAction.h"

UpdateFreeSpaceAction::UpdateFreeSpaceAction(FetchPagePort &fetchPagePort,
                                             WritePagePort &writePagePort,
                                             UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void UpdateFreeSpaceAction::execute(SpaceMapPageId spaceMapPageId, TuplePageId pageId,
                                    uint32_t updatedfFreeSpace) {
  SpaceMapPage spaceMapPage = fetchPagePort.fetchPage(spaceMapPageId);
  spaceMapPage.update(pageId, updatedfFreeSpace);
  writePagePort.writePage(spaceMapPageId, spaceMapPage);
  unpinPagePort.unpinPage(spaceMapPageId);
  return;
}
