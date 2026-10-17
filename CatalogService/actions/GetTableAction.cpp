#include "GetTableAction.h"

#include <stdexcept>
#include <unordered_set>

GetTableAction::GetTableAction(CatalogFetchPagePort &fetchPagePort,
                               CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

Table GetTableAction::execute(CatalogPageId rootPageId,
                              CatalogTableId tableId) {
  CatalogPageId currentPageId = rootPageId;
  std::unordered_set<CatalogPageId> visitedPageIds;

  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(currentPageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    CatalogPageId nextPageId;
    bool found = false;
    Table table{};
    try {
      found = page.containsTableId(tableId);
      if (found) {
        table = page.get(tableId);
      } else {
        nextPageId = page.getNextPageId();
      }
    } catch (...) {
      try {
        unpinPagePort.unpinPage(currentPageId);
      } catch (...) {
        // Preserve the original lookup failure.
      }
      throw;
    }

    unpinPagePort.unpinPage(currentPageId);
    if (found) {
      return table;
    }
    currentPageId = nextPageId;
  }

  throw std::runtime_error("Table not found");
}
