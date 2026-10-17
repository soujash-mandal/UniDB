#include "GetTableByNameAction.h"

#include <stdexcept>
#include <unordered_set>

GetTableByNameAction::GetTableByNameAction(CatalogFetchPagePort &fetchPagePort,
                                           CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

Table GetTableByNameAction::execute(CatalogPageId rootPageId,
                                    const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

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
      nextPageId = page.getNextPageId();
      if (page.containsTableName(tableName)) {
        table = page.getByName(tableName);
        found = true;
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
