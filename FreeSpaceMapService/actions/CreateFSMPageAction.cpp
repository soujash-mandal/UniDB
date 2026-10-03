#include "CreateFSMPageAction.h"

CreateFSMPageAction::CreateFSMPageAction(NewPagePort &newPagePort,
                                         WritePagePort &writePagePort,
                                         UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

FSMPageId CreateFSMPageAction::execute() {
  FSMPageId pageId = newPagePort.newPage();
  FSMPage page;
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return pageId;
}
