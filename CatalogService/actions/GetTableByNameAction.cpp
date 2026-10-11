#include "GetTableByNameAction.h"

#include <stdexcept>

GetTableByNameAction::GetTableByNameAction(
    CatalogFetchPagePort &fetchPagePort,
    CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

Table GetTableByNameAction::execute(CatalogPageId rootPageId,
                                    const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  CatalogPageId currentPageId = rootPageId;
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    const CatalogPageId nextPageId = page.getNextPageId();
    if (page.containsTableName(tableName)) {
      Table table = page.getByName(tableName);
      unpinPagePort.unpinPage(currentPageId);
      return table;
    }
    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }

  throw std::runtime_error("Table not found");
}
