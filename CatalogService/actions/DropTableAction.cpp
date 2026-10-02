#include "DropTableAction.h"

#include <stdexcept>

DropTableAction::DropTableAction(FetchPagePort &fetchPagePort,
                                 WritePagePort &writePagePort,
                                 UnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), writePagePort(writePagePort),
      unpinPagePort(unpinPagePort) {}

void DropTableAction::execute(CatalogPageId rootPageId,
                              CatalogTableId tableId) {

  CatalogPageId pageId = rootPageId;
  while (pageId != INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(pageId);
    try {
      page.get(tableId);
      page.remove(tableId);
      writePagePort.writePage(pageId, page);
      unpinPagePort.unpinPage(pageId);
      return;
    } catch (const std::runtime_error &) {
      CatalogPageId nextPageId = page.getNextPageId();
      unpinPagePort.unpinPage(pageId);
      pageId = nextPageId;
    }
  }
  throw std::runtime_error("Table not found");
}
