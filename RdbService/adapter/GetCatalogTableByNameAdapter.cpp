#include "GetCatalogTableByNameAdapter.h"

#include "../../CatalogService/domain/CatalogPage.h"

#include <stdexcept>

namespace {
RdbDataType fromCatalogType(DataType type) {
  switch (type) {
  case DataType::INT:
    return RdbDataType::INT;
  case DataType::BIGINT:
    return RdbDataType::BIGINT;
  case DataType::FLOAT:
    return RdbDataType::FLOAT;
  case DataType::DOUBLE:
    return RdbDataType::DOUBLE;
  case DataType::BOOLEAN:
    return RdbDataType::BOOLEAN;
  case DataType::VARCHAR:
    return RdbDataType::VARCHAR;
  case DataType::SMALLINT:
    return RdbDataType::SMALLINT;
  case DataType::NUMERIC:
    return RdbDataType::NUMERIC;
  case DataType::CHAR:
    return RdbDataType::CHAR;
  case DataType::TEXT:
    return RdbDataType::TEXT;
  }
  throw std::invalid_argument("Unsupported catalog column type");
}
RdbTableDefinition fromCatalogTable(const Table &table) {
  RdbTableDefinition result;
  result.tableId = table.tableId;
  result.name = table.name;
  result.firstSpaceMapPageId = table.firstFreeSpaceMapPageId;
  result.columns.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    result.columns.push_back(RdbColumn{
        column.name, fromCatalogType(column.type), column.size, column.nullable,
        column.numericPrecision, column.numericScale, column.characterLength});
  }
  return result;
}
} // namespace

GetCatalogTableByNameAdapter::GetCatalogTableByNameAdapter(
    GetTableByNameAction &action)
    : action(action) {}

RdbTableDefinition
GetCatalogTableByNameAdapter::getTableByName(uint32_t catalogRootPageId,
                                             const std::string &tableName) {
  return fromCatalogTable(action.execute(catalogRootPageId, tableName));
}
