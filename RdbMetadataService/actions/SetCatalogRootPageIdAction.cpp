#include "SetCatalogRootPageIdAction.h"

SetCatalogRootPageIdAction::SetCatalogRootPageIdAction(
    ReadRdbMetadataPagePort &readMetadataPagePort,
    WriteRdbMetadataPagePort &writeMetadataPagePort)
    : readMetadataPagePort(readMetadataPagePort),
      writeMetadataPagePort(writeMetadataPagePort) {}

void SetCatalogRootPageIdAction::execute(CatalogPageId metadataPageId,
                                         CatalogPageId catalogRootPageId) {
  RdbMetadataPage metadataPage = readMetadataPagePort.readPage(metadataPageId);
  metadataPage.setCatalogRootPageId(catalogRootPageId);
  writeMetadataPagePort.writePage(metadataPageId, metadataPage);
}
