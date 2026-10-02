#include "CreateTableAction.h"
#include <stdexcept>

CreateTableAction::CreateTableAction(
    FetchPagePort &fetchPagePort, NewPagePort &newPagePort,
    WritePagePort &writePagePort, UnpinPagePort &unpinPagePort,
    GetCatalogRootPagePort &getCatalogRootPagePort,
    AllocateTableIdPort &allocateTableIdPort)
    : fetchPagePort(fetchPagePort), newPagePort(newPagePort),
      writePagePort(writePagePort), unpinPagePort(unpinPagePort),
      getCatalogRootPagePort(getCatalogRootPagePort),
      allocateTableIdPort(allocateTableIdPort) {}

CatalogTableId CreateTableAction::execute(std::string name,
                                          std::vector<Column> columns) {
  // 1 : name must be non empty
  if (name.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  // 2: fetch root page id
  CatalogPageId rootPageId = getCatalogRootPagePort.getCatalogRootPageId();

  // 3 : starting Catalog Page
  CatalogPageId pageId = rootPageId;
  // 4: name must not exist in any catalog page
  while (pageId != INVALID_PAGE_ID) {
    CatalogPage page = fetchPagePort.fetchPage(pageId);
    unpinPagePort.unpinPage(pageId);
    if (page.containsTableName(name)) {
      throw "table already exist";
    } else {
      pageId = page.getNextPageId();
    }
  }

  // 5 : restart again to find suitable page to create table
  pageId = rootPageId;
  CatalogPage page = fetchPagePort.fetchPage(pageId);
  CatalogTableId tableId = allocateTableIdPort.allocateTableId();

  // 6 : Create Table Object which we will gonna insert a table
  Table table;
  table.tableId = tableId;
  table.name = name;
  table.columns = columns;
  table.firstFreeSpaceMapPageId = INVALID_PAGE_ID;

  // 7 : Loop Untill we find a suitable catalogpage to store new Table
  while (!page.hasSpace(table)) {
    CatalogPageId nextPageId = page.getNextPageId();
    if (nextPageId == INVALID_PAGE_ID) {
      CatalogPageId newPageId = newPagePort.newPage();
      page.setNextPageId(newPageId);
      nextPageId = newPageId;
      writePagePort.writePage(pageId, page);
    }

    unpinPagePort.unpinPage(pageId);
    page = fetchPagePort.fetchPage(nextPageId);
    pageId = nextPageId;
  }

  // 8: Found a CatalogPage with Enough space create Table here
  page.insert(table);
  writePagePort.writePage(pageId, page);
  unpinPagePort.unpinPage(pageId);
}
