#include "UpdateCatalogTableAdapter.h"

#include "../../CatalogService/domain/CatalogPage.h"

#include <stdexcept>

namespace {
DataType toCatalogType(RdbDataType type) {
  switch (type) {
  case RdbDataType::INT:
    return DataType::INT;
  case RdbDataType::BIGINT:
    return DataType::BIGINT;
  case RdbDataType::FLOAT:
    return DataType::FLOAT;
  case RdbDataType::DOUBLE:
    return DataType::DOUBLE;
  case RdbDataType::BOOLEAN:
    return DataType::BOOLEAN;
  case RdbDataType::VARCHAR:
    return DataType::VARCHAR;
  }
  throw std::invalid_argument("Unsupported RDB column type");
}
Table toCatalogTable(const RdbTableDefinition &table,
                     uint32_t firstSpaceMapPageId) {
  Table result{};
  result.tableId = table.tableId;
  result.name = table.name;
  result.firstFreeSpaceMapPageId = firstSpaceMapPageId;
  result.columns.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    result.columns.push_back(Column{column.name, toCatalogType(column.type),
                                    column.size, column.nullable});
  }
  return result;
}
} // namespace

UpdateCatalogTableAdapter::UpdateCatalogTableAdapter(UpdateTableAction &action)
    : action(action) {}

void UpdateCatalogTableAdapter::updateTable(uint32_t catalogRootPageId,
                                            const RdbTableDefinition &table,
                                            uint32_t firstSpaceMapPageId) {
  action.execute(catalogRootPageId, toCatalogTable(table, firstSpaceMapPageId));
}
