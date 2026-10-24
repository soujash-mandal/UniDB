#include "CatalogTableAdapter.h"

#include "../../CatalogService/domain/CatalogPage.h"

#include <stdexcept>
#include <utility>

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
  }
  throw std::invalid_argument("Unsupported catalog column type");
}

std::vector<Column> toCatalogColumns(const std::vector<RdbColumn> &columns) {
  std::vector<Column> result;
  result.reserve(columns.size());
  for (const auto &column : columns) {
    result.push_back(
        Column{column.name, toCatalogType(column.type), column.size,
               column.nullable});
  }
  return result;
}

RdbTableDefinition fromCatalogTable(const Table &table) {
  RdbTableDefinition result;
  result.tableId = table.tableId;
  result.name = table.name;
  result.firstSpaceMapPageId = table.firstFreeSpaceMapPageId;
  result.columns.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    result.columns.push_back(RdbColumn{column.name, fromCatalogType(column.type),
                                       column.size, column.nullable});
  }
  return result;
}

Table toCatalogTable(const RdbTableDefinition &table) {
  Table result{};
  result.tableId = table.tableId;
  result.name = table.name;
  result.columns = toCatalogColumns(table.columns);
  result.firstFreeSpaceMapPageId = table.firstSpaceMapPageId;
  return result;
}
} // namespace

CatalogTableAdapter::CatalogTableAdapter(
    CreateTableAction &createTableAction,
    GetTableByNameAction &getTableByNameAction,
    UpdateTableAction &updateTableAction)
    : createTableAction(createTableAction),
      getTableByNameAction(getTableByNameAction),
      updateTableAction(updateTableAction) {}

uint32_t CatalogTableAdapter::createTable(
    uint32_t catalogRootPageId, uint32_t tableId,
    const RdbTableDefinition &table) {
  return createTableAction.execute(catalogRootPageId, tableId, table.name,
                                   toCatalogColumns(table.columns));
}

RdbTableDefinition CatalogTableAdapter::getTableByName(
    uint32_t catalogRootPageId, const std::string &tableName) {
  return fromCatalogTable(
      getTableByNameAction.execute(catalogRootPageId, tableName));
}

void CatalogTableAdapter::updateTable(uint32_t catalogRootPageId,
                                      const RdbTableDefinition &table) {
  auto catalogTable = toCatalogTable(table);
  updateTableAction.execute(catalogRootPageId, std::move(catalogTable));
}
