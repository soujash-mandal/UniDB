#include "CreateRootCatalogPageAction.h"

CreateRootCatalogPageAction::CreateRootCatalogPageAction(
    CatalogNewPagePort &newPagePort, CatalogWritePagePort &writePagePort,
    CatalogUnpinPagePort &unpinPagePort)
    : newPagePort(newPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

CatalogPageId CreateRootCatalogPageAction::execute() {
  CatalogPageId catalogRootPageId = newPagePort.newPage();
  CatalogPage page;
  try {
    writePagePort.writePage(catalogRootPageId, page);
  } catch (...) {
    try {
      unpinPagePort.unpinPage(catalogRootPageId);
    } catch (...) {
      // Preserve the original page initialization failure.
    }
    throw;
  }
  unpinPagePort.unpinPage(catalogRootPageId);
  return catalogRootPageId;
}
