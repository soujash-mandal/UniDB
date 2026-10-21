#pragma once

#include "../../domain/CatalogPage.h"

#include <string>
#include <utility>

inline Table makeCatalogPageTestTable(CatalogTableId id, std::string name) {
  Table table;
  table.tableId = id;
  table.name = std::move(name);
  table.firstFreeSpaceMapPageId = CATALOG_INVALID_PAGE_ID;
  return table;
}

inline void insertThreeCatalogPageTestTables(CatalogPage &page) {
  page.insert(makeCatalogPageTestTable(1, "first"));
  page.insert(makeCatalogPageTestTable(2, "middle"));
  page.insert(makeCatalogPageTestTable(3, "last"));
}
