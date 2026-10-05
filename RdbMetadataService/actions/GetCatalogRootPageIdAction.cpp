#include "GetCatalogRootPageIdAction.h"

GetCatalogRootPageIdAction::GetCatalogRootPageIdAction(
    FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

CatalogPageId GetCatalogRootPageIdAction::execute(CatalogPageId pageId) {
  RdbMetadataPage page = fetchPagePort.fetchPage(pageId);
  CatalogPageId catalogRootPageId = page.getCatalogRootPageId();
  unpinPagePort.unpinPage(pageId);
  return catalogRootPageId;
}
