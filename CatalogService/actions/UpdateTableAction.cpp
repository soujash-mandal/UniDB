#include "UpdateTableAction.h"

#include <stdexcept>
#include <unordered_set>

UpdateTableAction::UpdateTableAction(CatalogFetchPagePort &fetchPagePort,
                                     CatalogWritePagePort &writePagePort,
                                     CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void UpdateTableAction::execute(CatalogPageId rootPageId, Table table) {
  if (table.name.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  // First validate the name across the complete chain before mutating a page.
  CatalogPageId currentPageId = rootPageId;
  std::unordered_set<CatalogPageId> visitedPageIds;
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(currentPageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    CatalogPageId nextPageId;
    try {
      nextPageId = page.getNextPageId();
      if (page.containsTableName(table.name)) {
        const Table namedTable = page.getByName(table.name);
        if (namedTable.tableId != table.tableId) {
          throw std::runtime_error("Table name already exists");
        }
      }
    } catch (...) {
      try {
        unpinPagePort.unpinPage(currentPageId);
      } catch (...) {
        // Preserve the original validation failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(currentPageId);
    currentPageId = nextPageId;
  }

  // Second pass finds and updates the target only after uniqueness is known.
  currentPageId = rootPageId;
  visitedPageIds.clear();
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(currentPageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

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
