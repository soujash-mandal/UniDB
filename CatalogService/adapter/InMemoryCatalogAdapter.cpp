#include "InMemoryCatalogAdapter.h"

void InMemoryCatalogAdapter::createTable(TableId tableId,
                                         const std::string &tableName) {
  tables[tableName] = tableId;
}
