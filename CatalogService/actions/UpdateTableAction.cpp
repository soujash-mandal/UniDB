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
    const CatalogPageId nextPageId = page.getNextPageId();
    if (page.containsTableId(table.tableId)) {
      page.update(table);
      writePagePort.writePage(currentPageId, page);
      unpinPagePort.unpinPage(currentPageId);
      return;
    }
    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }
  throw std::runtime_error("Table not found");
}
