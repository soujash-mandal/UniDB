#include "GetTableByNameAction.h"
#include "../domain/CatalogPage.h"

#include <stdexcept>

GetTableByNameAction::GetTableByNameAction(CatalogFetchPagePort &fetchPagePort,
                                           CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

CatalogTableDefinition GetTableByNameAction::execute(
    CatalogPageId rootPageId, const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  CatalogPageId currentPageId = rootPageId;
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    const CatalogPageId nextPageId = page.getNextPageId();
    if (page.containsTableName(tableName)) {
      const Table stored = page.getByName(tableName);
      unpinPagePort.unpinPage(currentPageId);
      return CatalogTableDefinition{stored.tableId, stored.name, stored.columns};
    }
    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }

  throw std::runtime_error("Table not found");
}
