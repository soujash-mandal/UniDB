#include "InitializeFSMPageAction.h"

InitializeFSMPageAction::InitializeFSMPageAction(NewPagePort &newPagePort,
                                                 WritePagePort &writePagePort,
                                                 UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

FSMPageId InitializeFSMPageAction::execute() {
  FSMPageId pageId = newPagePort.newPage();
  FSMPage page;
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return pageId;
}
