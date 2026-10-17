#include "CreateTableAction.h"
#include "../domain/CatalogPage.h"

#include <stdexcept>
#include <unordered_set>

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

  Table table;
  table.tableId = tableId;
  table.name = name;
  table.columns = columns;
  table.firstFreeSpaceMapPageId = CATALOG_INVALID_PAGE_ID;
  table.firstTuplePageId = CATALOG_INVALID_PAGE_ID;

  CatalogPage emptyPage;
  if (!emptyPage.hasSpace(table)) {
    throw std::length_error("Table metadata is too large for a catalog page");
  }

  CatalogPageId pageId = rootPageId;
  std::unordered_set<CatalogPageId> visitedPageIds;
  while (pageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(pageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(pageId);
    CatalogPageId nextPageId;
    bool duplicate;
    try {
      duplicate = page.containsTableName(name);
      nextPageId = page.getNextPageId();
    } catch (...) {
      try {
        unpinPagePort.unpinPage(pageId);
      } catch (...) {
        // Preserve the original catalog inspection failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(pageId);
    if (duplicate) {
      throw std::runtime_error("Table already exists");
    }
    pageId = nextPageId;
  }

  pageId = rootPageId;
  visitedPageIds.clear();
  while (pageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(pageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(pageId);
    CatalogPageId nextPageId;
    bool hasSpace;
    try {
      hasSpace = page.hasSpace(table);
      nextPageId = page.getNextPageId();
    } catch (...) {
      try {
        unpinPagePort.unpinPage(pageId);
      } catch (...) {
        // Preserve the original catalog inspection failure.
      }
      throw;
    }

    if (hasSpace) {
      try {
        page.insert(table);
        writePagePort.writePage(pageId, page);
      } catch (...) {
        try {
          unpinPagePort.unpinPage(pageId);
        } catch (...) {
          // Preserve the original insertion or write failure.
        }
        throw;
      }
      unpinPagePort.unpinPage(pageId);
      return tableId;
    }

    if (nextPageId != CATALOG_INVALID_PAGE_ID) {
      unpinPagePort.unpinPage(pageId);
      pageId = nextPageId;
      continue;
    }

    // Release the tail before allocation so a one-frame pool can evict it.
    unpinPagePort.unpinPage(pageId);
    const CatalogPageId newPageId = newPagePort.newPage();
    CatalogPage newPage;
    try {
      writePagePort.writePage(newPageId, newPage);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(newPageId);
      } catch (...) {
        // Preserve the original page initialization failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(newPageId);

    CatalogPage tailPage = fetchPagePort.fetchPage(pageId);
    try {
      tailPage.setNextPageId(newPageId);
      writePagePort.writePage(pageId, tailPage);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(pageId);
      } catch (...) {
        // Preserve the original page-link failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(pageId);
    pageId = newPageId;
  }

  throw std::runtime_error("Catalog page chain is invalid");
}
