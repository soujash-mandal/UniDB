#include "CreateRootCatalogPageAction.h"

CreateRootCatalogPageAction::CreateRootCatalogPageAction(
    NewPagePort &newPagePort, WritePagePort &writePagePort,
    UnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CatalogPageId CreateRootCatalogPageAction::execute() {
  CatalogPageId catalogRootPageId = newPagePort.newPage();
  CatalogPage page;
  writePagePort.writePage(catalogRootPageId, page);
  unpinPagePort.unpinPage(catalogRootPageId);
  return catalogRootPageId;
}
