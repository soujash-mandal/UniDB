#include "CreateRootCatalogPageAction.h"

CreateRootCatalogPageAction::CreateRootCatalogPageAction(
    CatalogNewPagePort &newPagePort, CatalogWritePagePort &writePagePort,
    CatalogUnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CatalogPageId CreateRootCatalogPageAction::execute() {
  CatalogPageId catalogRootPageId = newPagePort.newPage();
  CatalogPage page;
  writePagePort.writePage(catalogRootPageId, page);
  unpinPagePort.unpinPage(catalogRootPageId);
  return catalogRootPageId;
}
