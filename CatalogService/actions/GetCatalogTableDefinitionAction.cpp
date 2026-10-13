#include "GetCatalogTableDefinitionAction.h"
#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <stdexcept>

GetCatalogTableDefinitionAction::GetCatalogTableDefinitionAction(
    CatalogFetchPagePort &fetchPagePort,
    CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort) {}

CatalogTableDefinition GetCatalogTableDefinitionAction::execute(
    CatalogPageId rootPageId, const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  CatalogPageId pageId = rootPageId;
  while (pageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(pageId);
    const CatalogPageId nextPageId = page.getNextPageId();
    if (page.containsTableName(tableName)) {
      const Table stored = page.getByName(tableName);
      unpinPagePort.unpinPage(pageId);
      return CatalogTableDefinition{stored.tableId, stored.name, stored.columns,
                                   stored.firstTuplePageId};
    }
    unpinPagePort.unpinPage(pageId);
    pageId = nextPageId;
  }

  throw std::runtime_error("Table not found");
}
