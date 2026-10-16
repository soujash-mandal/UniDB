#include "UpdateTableAction.h"

#include <stdexcept>

UpdateTableAction::UpdateTableAction(CatalogFetchPagePort &fetchPagePort,
                                     CatalogWritePagePort &writePagePort,
                                     CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void UpdateTableAction::execute(CatalogPageId rootPageId, Table table) {
  CatalogPageId currentPageId = rootPageId;
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    CatalogPageId nextPageId;
    bool found = false;
    try {
      nextPageId = page.getNextPageId();
      if (page.containsTableId(table.tableId)) {
        page.update(table);
        writePagePort.writePage(currentPageId, page);
        found = true;
      }
    } catch (...) {
      try {
        unpinPagePort.unpinPage(currentPageId);
      } catch (...) {
        // Preserve the original update failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(currentPageId);
    if (found) {
      return;
    }
    currentPageId = nextPageId;
  }
  throw std::runtime_error("Table not found");
}
