#include "CreateRdbMetadataPageAction.h"

CreateRdbMetadataPageAction::CreateRdbMetadataPageAction(
    NewPagePort &newPagePort,
    WritePagePort &writePagePort,
    UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort),
      writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CatalogPageId CreateRdbMetadataPageAction::execute() {
  CatalogPageId pageId = newPagePort.newPage();
  RdbMetadataPage page;
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return pageId;
}
