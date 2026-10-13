#include "UpdateCatalogTupleRootAction.h"
#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <stdexcept>

UpdateCatalogTupleRootAction::UpdateCatalogTupleRootAction(
    CatalogFetchPagePort &fetchPagePort, CatalogWritePagePort &writePagePort,
    CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void UpdateCatalogTupleRootAction::execute(CatalogPageId rootPageId,
                                           CatalogTableId tableId,
                                           CatalogPageId tupleRootPageId) {
  CatalogPageId pageId = rootPageId;
  while (pageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(pageId);
    const CatalogPageId nextPageId = page.getNextPageId();
    if (page.containsTableId(tableId)) {
      Table stored = page.get(tableId);
      stored.firstTuplePageId = tupleRootPageId;
      page.update(stored);
      writePagePort.writePage(pageId, page);
      unpinPagePort.unpinPage(pageId);
      return;
    }
    unpinPagePort.unpinPage(pageId);
    pageId = nextPageId;
  }
  throw std::runtime_error("Table not found");
}
