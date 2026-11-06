#include "GetCatalogTablesAdapter.h"
#include "../../CatalogService/domain/CatalogPage.h"
#include <stdexcept>
namespace {
RdbDataType toRdbType(DataType type) {
  switch (type) {
  case DataType::INT: return RdbDataType::INT;
  case DataType::BIGINT: return RdbDataType::BIGINT;
  case DataType::FLOAT: return RdbDataType::FLOAT;
  case DataType::DOUBLE: return RdbDataType::DOUBLE;
  case DataType::BOOLEAN: return RdbDataType::BOOLEAN;
  case DataType::VARCHAR: return RdbDataType::VARCHAR;
  }
  throw std::invalid_argument("Unsupported catalog column type");
}
RdbTableDefinition convert(const Table &table) {
  RdbTableDefinition result;
  result.tableId = table.tableId;
  result.name = table.name;
  result.firstSpaceMapPageId = table.firstFreeSpaceMapPageId;
  result.columns.reserve(table.columns.size());
  for (const auto &column : table.columns)
    result.columns.push_back({column.name, toRdbType(column.type), column.size, column.nullable});
  return result;
}
}
GetCatalogTablesAdapter::GetCatalogTablesAdapter(GetAllTablesAction &action) : action(action) {}
std::vector<RdbTableDefinition> GetCatalogTablesAdapter::getTables(uint32_t rootPageId) {
  const auto tables = action.execute(rootPageId);
  std::vector<RdbTableDefinition> result;
  result.reserve(tables.size());
  for (const auto &table : tables) result.push_back(convert(table));
  return result;
}
