#include "RdbCreateTableAction.h"

#include <stdexcept>
#include <unordered_set>

RdbCreateTableAction::RdbCreateTableAction(RdbMetadataPort &metadataPort,
                                           CatalogTablePort &catalogTablePort)
    : metadataPort(metadataPort), catalogTablePort(catalogTablePort) {}

uint32_t RdbCreateTableAction::execute(const std::string &tableName,
                                       const std::vector<RdbColumn> &columns) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }
  if (columns.empty()) {
    throw std::invalid_argument("A table must have at least one column");
  }

  std::unordered_set<std::string> columnNames;
  for (const auto &column : columns) {
    if (column.name.empty()) {
      throw std::invalid_argument("Column name cannot be empty");
    }
    if (!columnNames.insert(column.name).second) {
      throw std::invalid_argument("Duplicate column name: " + column.name);
    }
  }

  const CatalogPageId catalogRootPageId = metadataPort.getCatalogRootPageId();
  const uint32_t tableId = metadataPort.allocateNextTableId();

  RdbTableDefinition table;
  table.tableId = tableId;
  table.name = tableName;
  table.columns = columns;
  catalogTablePort.createTable(catalogRootPageId, tableId, table);
  return tableId;
}
