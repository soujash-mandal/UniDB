#include "CreateSpaceMapPageAction.h"

CreateSpaceMapPageAction::CreateSpaceMapPageAction(
    NewPagePort &newPagePort, SpaceMapWritePagePort &writePagePort,
    UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

SpaceMapPageId CreateSpaceMapPageAction::execute() {
  SpaceMapPageId pageId = newPagePort.newPage();
  SpaceMapPage page;
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return pageId;
}
