#include "CreateRdbMetadataPageAction.h"

CreateRdbMetadataPageAction::CreateRdbMetadataPageAction(
    RdbMetadataNewPagePort &newPagePort,
    RdbMetadataWritePagePort &writePagePort,
    RdbMetadataUnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CatalogPageId CreateRdbMetadataPageAction::execute() {
  CatalogPageId pageId = newPagePort.newPage();
  RdbMetadataPage page;
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return pageId;
}
