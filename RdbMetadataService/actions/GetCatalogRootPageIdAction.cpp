#include "GetCatalogRootPageIdAction.h"

GetCatalogRootPageIdAction::GetCatalogRootPageIdAction(
    ReadRdbMetadataPagePort &readMetadataPagePort)
    : readMetadataPagePort(readMetadataPagePort) {}

CatalogPageId GetCatalogRootPageIdAction::execute(CatalogPageId pageId) {
  RdbMetadataPage metadataPage = readMetadataPagePort.readPage(pageId);
  return metadataPage.getCatalogRootPageId();
}
