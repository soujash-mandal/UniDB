#include "CreateTableAction.h"

#include <stdexcept>
#include <unordered_set>

namespace RdbService {
namespace {
void validateTableDefinition(const RdbTableDefinition &table) {
  if (table.name.empty()) {
    throw std::invalid_argument("Table name must not be empty");
  }
  if (table.columns.empty()) {
    throw std::invalid_argument("Table must contain at least one column");
  }

  std::unordered_set<std::string> columnNames;
  for (const auto &column : table.columns) {
    if (column.name.empty()) {
      throw std::invalid_argument("Column name must not be empty");
    }
    if (!columnNames.insert(column.name).second) {
      throw std::invalid_argument("Column names must be unique");
    }
    if (column.type == RdbDataType::VARCHAR && column.size == 0) {
      throw std::invalid_argument("VARCHAR columns must have a positive size");
    }
  }
}
} // namespace

CreateTableAction::CreateTableAction(
    GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
    AllocateNextTableIdPort &allocateNextTableIdPort,
    CreateCatalogTablePort &createCatalogTablePort)
    : getCatalogRootPageIdPort(getCatalogRootPageIdPort),
      allocateNextTableIdPort(allocateNextTableIdPort),
      createCatalogTablePort(createCatalogTablePort) {}

uint32_t CreateTableAction::execute(uint32_t metadataPageId,
                                    RdbTableDefinition table) {
  validateTableDefinition(table);

  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  const uint32_t tableId =
      allocateNextTableIdPort.allocateNextTableId(metadataPageId);
  table.tableId = tableId;

  return createCatalogTablePort.createTable(catalogRootPageId, tableId, table);
}

} // namespace RdbService
