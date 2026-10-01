#include "AllocateNextTableIdAction.h"

AllocateNextTableIdAction::AllocateNextTableIdAction(
    ReadRdbMetadataPagePort &readMetadataPagePort,
    WriteRdbMetadataPagePort &writeMetadataPagePort)
    : readMetadataPagePort(readMetadataPagePort),
      writeMetadataPagePort(writeMetadataPagePort) {}

CatalogTableId AllocateNextTableIdAction::execute(CatalogPageId pageId) {
  RdbMetadataPage metadataPage = readMetadataPagePort.readPage(pageId);
  CatalogTableId tableId = metadataPage.getNextTableId();
  metadataPage.setNextTableId(tableId + 1);
  writeMetadataPagePort.writePage(pageId, metadataPage);
  return tableId;
}
