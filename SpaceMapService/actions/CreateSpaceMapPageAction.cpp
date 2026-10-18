#include "CreateSpaceMapPageAction.h"

CreateSpaceMapPageAction::CreateSpaceMapPageAction(NewPagePort &newPagePort,
                                                   WritePagePort &writePagePort,
                                                   UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

FSMPageId CreateSpaceMapPageAction::execute() {
  FSMPageId pageId = newPagePort.newPage();
  FSMPage page;
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return pageId;
}
