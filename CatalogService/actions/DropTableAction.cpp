#include "DropTableAction.h"
#include "../domain/CatalogPage.h"

#include <stdexcept>
#include <unordered_set>

DropTableAction::DropTableAction(CatalogFetchPagePort &fetchPagePort,
                                 CatalogWritePagePort &writePagePort,
                                 CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void DropTableAction::execute(CatalogPageId rootPageId,
                              CatalogTableId tableId) {
  CatalogPageId pageId = rootPageId;
  std::unordered_set<CatalogPageId> visitedPageIds;

  while (pageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(pageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(pageId);
    CatalogPageId nextPageId;
    bool found = false;
    try {
      found = page.containsTableId(tableId);
      if (found) {
        page.remove(tableId);
        writePagePort.writePage(pageId, page);
      } else {
        nextPageId = page.getNextPageId();
      }
    } catch (...) {
      try {
        unpinPagePort.unpinPage(pageId);
      } catch (...) {
        // Preserve the original lookup or write failure.
      }
      throw;
    }

    unpinPagePort.unpinPage(pageId);
    if (found) {
      return;
    }
    pageId = nextPageId;
  }

  throw std::runtime_error("Table not found");
}
