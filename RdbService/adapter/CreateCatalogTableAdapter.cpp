#include "CreateCatalogTableAdapter.h"

#include "../../CatalogService/domain/CatalogPage.h"

#include <stdexcept>
#include <vector>

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
  case RdbDataType::SMALLINT:
    return DataType::SMALLINT;
  case RdbDataType::NUMERIC:
    return DataType::NUMERIC;
  case RdbDataType::CHAR:
    return DataType::CHAR;
  case RdbDataType::TEXT:
    return DataType::TEXT;
  case RdbDataType::DATE:
    return DataType::DATE;
  case RdbDataType::TIME:
    return DataType::TIME;
  case RdbDataType::TIME_WITH_TIME_ZONE:
    return DataType::TIME_WITH_TIME_ZONE;
  case RdbDataType::TIMESTAMP:
    return DataType::TIMESTAMP;
  case RdbDataType::TIMESTAMP_WITH_TIME_ZONE:
    return DataType::TIMESTAMP_WITH_TIME_ZONE;
  case RdbDataType::INTERVAL:
    return DataType::INTERVAL;
  }
  throw std::invalid_argument("Unsupported RDB column type");
}
std::vector<Column> toCatalogColumns(const std::vector<RdbColumn> &columns) {
  std::vector<Column> result;
  result.reserve(columns.size());
  for (const auto &column : columns) {
    result.push_back(Column{
        column.name, toCatalogType(column.type), column.size, column.nullable,
        column.numericPrecision, column.numericScale, column.characterLength,
        column.temporalPrecision});
  }
  return result;
}
} // namespace

CreateCatalogTableAdapter::CreateCatalogTableAdapter(CreateTableAction &action)
    : action(action) {}

uint32_t
CreateCatalogTableAdapter::createTable(uint32_t catalogRootPageId,
                                       uint32_t tableId,
                                       const RdbTableDefinition &table) {
  return action.execute(catalogRootPageId, tableId, table.name,
                        toCatalogColumns(table.columns));
}
