#include "GetTableAction.h"

#include <stdexcept>

GetTableAction::GetTableAction(CatalogFetchPagePort &fetchPagePort,
                               CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

Table GetTableAction::execute(CatalogPageId rootPageId,
                              CatalogTableId tableId) {

  CatalogPageId currentPageId = rootPageId;

  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    if (page.containsTableId(tableId)) {
      Table table = page.get(tableId);
      unpinPagePort.unpinPage(currentPageId);
      return table;
    } else {
      CatalogPageId nextPageId = page.getNextPageId();
      unpinPagePort.unpinPage(currentPageId);
      currentPageId = nextPageId;
    }
  }

  throw std::runtime_error("Table not found");
}
