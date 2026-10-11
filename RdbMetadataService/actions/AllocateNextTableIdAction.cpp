#include "AllocateNextTableIdAction.h"

AllocateNextTableIdAction::AllocateNextTableIdAction(
    RdbMetadataFetchPagePort &fetchPagePort,
    RdbMetadataWritePagePort &writePagePort,
    RdbMetadataUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CatalogTableId AllocateNextTableIdAction::execute(CatalogPageId pageId) {
  RdbMetadataPage page = fetchPagePort.fetchPage(pageId);
  CatalogTableId tableId = page.getNextTableId();
  page.setNextTableId(tableId + 1);
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
  return tableId;
}
