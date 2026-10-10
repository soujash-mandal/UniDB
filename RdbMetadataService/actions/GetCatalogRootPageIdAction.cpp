#include "GetCatalogRootPageIdAction.h"

GetCatalogRootPageIdAction::GetCatalogRootPageIdAction(
    RdbMetadataFetchPagePort &fetchPagePort,
    RdbMetadataUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

CatalogPageId
GetCatalogRootPageIdAction::execute(CatalogPageId metadataPageId) {
  const RdbMetadataPage page = fetchPagePort.fetchPage(metadataPageId);
  const CatalogPageId catalogRootPageId = page.getCatalogRootPageId();
  unpinPagePort.unpinPage(metadataPageId);
  return catalogRootPageId;
}
