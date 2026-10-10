#include "CreateTableAction.h"
#include <stdexcept>

CreateTableAction::CreateTableAction(CatalogFetchPagePort &fetchPagePort,
                                     CatalogNewPagePort &newPagePort,
                                     CatalogWritePagePort &writePagePort,
                                     CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), newPagePort(newPagePort),
      writePagePort(writePagePort), unpinPagePort(unpinPagePort) {}

CatalogTableId CreateTableAction::execute(CatalogPageId rootPageId,
                                          CatalogTableId tableId,
                                          std::string name,
                                          std::vector<Column> columns) {
  if (name.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  CatalogPageId pageId = rootPageId;

  while (pageId != CATALOG_INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(pageId);
    unpinPagePort.unpinPage(pageId);
    if (page.containsTableName(name)) {
      throw std::runtime_error("Table already exists");
    } else {
      pageId = page.getNextPageId();
    }
  }

  pageId = rootPageId;
  CatalogPage page = fetchPagePort.fetchPage(pageId);

  Table table;
  table.tableId = tableId;
  table.name = name;
  table.columns = columns;
  table.firstFreeSpaceMapPageId = CATALOG_INVALID_PAGE_ID;

  while (!page.hasSpace(table)) {
    CatalogPageId nextPageId = page.getNextPageId();
    if (nextPageId == CATALOG_INVALID_PAGE_ID) {
      CatalogPageId newPageId = newPagePort.newPage();
      page.setNextPageId(newPageId);
      nextPageId = newPageId;
      writePagePort.writePage(pageId, page);
    }

    unpinPagePort.unpinPage(pageId);
    page = fetchPagePort.fetchPage(nextPageId);
    pageId = nextPageId;
  }

  page.insert(table);
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);

  return tableId;
}
