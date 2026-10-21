#include "UpdateTableAction.h"

#include <stdexcept>
#include <unordered_set>

UpdateTableAction::UpdateTableAction(CatalogFetchPagePort &fetchPagePort,
                                     CatalogNewPagePort &newPagePort,
                                     CatalogWritePagePort &writePagePort,
                                     CatalogUnpinPagePort &unpinPagePort)
    : fetchPagePort(fetchPagePort), newPagePort(newPagePort),
      writePagePort(writePagePort), unpinPagePort(unpinPagePort) {}

void UpdateTableAction::execute(CatalogPageId rootPageId, Table table) {
  if (table.name.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  CatalogPageId currentPageId = rootPageId;
  std::unordered_set<CatalogPageId> visitedPageIds;
  bool targetFound = false;
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(currentPageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    CatalogPageId nextPageId;
    try {
      nextPageId = page.getNextPageId();
      if (page.containsTableId(table.tableId)) {
        targetFound = true;
      }
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

  if (!targetFound) {
    throw std::runtime_error("Table not found");
  }

  CatalogPageId originalPageId = CATALOG_INVALID_PAGE_ID;
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
        CatalogPage withoutTarget = page;
        withoutTarget.remove(table.tableId);
        if (withoutTarget.hasSpace(table)) {
          page.update(table);
          writePagePort.writePage(currentPageId, page);
        } else {
          // Keep the old record until the replacement has been persisted.
          originalPageId = currentPageId;
        }
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
      if (originalPageId == CATALOG_INVALID_PAGE_ID) {
        return;
      }
      break;
    }
    currentPageId = nextPageId;
  }

  if (originalPageId == CATALOG_INVALID_PAGE_ID) {
    throw std::runtime_error("Table not found");
  }

  const auto removeOriginalAfterReplacementIsSaved = [&]() {
    CatalogPage original = fetchPagePort.fetchPage(originalPageId);
    try {
      original.remove(table.tableId);
      writePagePort.writePage(originalPageId, original);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(originalPageId);
      } catch (...) {
        // Preserve the original removal failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(originalPageId);
  };

  // Search for an existing destination, excluding the page that still owns
  // the original record. Persist the replacement before removing that record.
  currentPageId = rootPageId;
  visitedPageIds.clear();
  while (currentPageId != CATALOG_INVALID_PAGE_ID) {
    if (!visitedPageIds.insert(currentPageId).second) {
      throw std::runtime_error("Catalog page chain is invalid");
    }

    CatalogPage page = fetchPagePort.fetchPage(currentPageId);
    CatalogPageId nextPageId;
    bool hasSpace = false;
    try {
      nextPageId = page.getNextPageId();
      if (currentPageId != originalPageId) {
        hasSpace = page.hasSpace(table);
        if (hasSpace) {
          page.insert(table);
          writePagePort.writePage(currentPageId, page);
        }
      }
    } catch (...) {
      try {
        unpinPagePort.unpinPage(currentPageId);
      } catch (...) {
        // Preserve the original relocation failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(currentPageId);

    if (hasSpace) {
      removeOriginalAfterReplacementIsSaved();
      return;
    }

    if (nextPageId != CATALOG_INVALID_PAGE_ID) {
      currentPageId = nextPageId;
      continue;
    }

    const CatalogPageId tailPageId = currentPageId;
    const CatalogPageId newPageId = newPagePort.newPage();
    CatalogPage newPage;
    try {
      newPage.insert(table);
      writePagePort.writePage(newPageId, newPage);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(newPageId);
      } catch (...) {
        // Preserve the original allocation or write failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(newPageId);

    CatalogPage tailPage = fetchPagePort.fetchPage(tailPageId);
    try {
      tailPage.setNextPageId(newPageId);
      writePagePort.writePage(tailPageId, tailPage);
    } catch (...) {
      try {
        unpinPagePort.unpinPage(tailPageId);
      } catch (...) {
        // Preserve the original page-link failure.
      }
      throw;
    }
    unpinPagePort.unpinPage(tailPageId);

    removeOriginalAfterReplacementIsSaved();
    return;
  }

  throw std::runtime_error("Catalog page chain is invalid");
}
