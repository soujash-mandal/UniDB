#include "RdbServiceAdapter.h"

#include "../../RdbService/domain/RdbTable.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace QueryExecutionService {
namespace {

RdbDataType toRdbType(unidb::sql::SqlType type) {
  switch (type) {
  case unidb::sql::SqlType::Int:
    return RdbDataType::INT;
  case unidb::sql::SqlType::Text:
    return RdbDataType::VARCHAR;
  }
  throw std::invalid_argument("SQL type is outside the V1 supported subset");
}

unidb::sql::SqlType fromRdbType(RdbDataType type) {
  switch (type) {
  case RdbDataType::INT:
    return unidb::sql::SqlType::Int;
  case RdbDataType::VARCHAR:
    return unidb::sql::SqlType::Text;
  default:
    throw std::invalid_argument(
        "Table schema contains a type outside the V1 SQL subset");
  }
}

} // namespace

RdbServiceAdapter::RdbServiceAdapter(
    uint32_t metadataPageId, RdbService::CreateTableAction &createTableAction,
    RdbService::GetTableDefinitionAction &getTableDefinitionAction,
    RdbService::InsertRowAction &insertRowAction,
    RdbService::SelectAllRowsAction &selectAllRowsAction)
    : metadataPageId(metadataPageId), createTableAction(createTableAction),
      getTableDefinitionAction(getTableDefinitionAction),
      insertRowAction(insertRowAction),
      selectAllRowsAction(selectAllRowsAction) {}

uint32_t RdbServiceAdapter::createTable(
    const unidb::sql::CreateTableStatement &statement) {
  RdbTableDefinition table;
  table.name = statement.tableName;
  table.columns.reserve(statement.columns.size());
  for (const auto &column : statement.columns) {
    const auto type = toRdbType(column.type);
    const uint16_t size =
        type == RdbDataType::VARCHAR ? std::numeric_limits<uint16_t>::max() : 0;
    table.columns.push_back(RdbColumn{column.name, type, size, false});
  }
  return createTableAction.execute(metadataPageId, std::move(table));
}

std::vector<unidb::sql::ColumnDefinition>
RdbServiceAdapter::getTableDefinition(const std::string &tableName) {
  const auto table =
      getTableDefinitionAction.execute(metadataPageId, tableName);
  std::vector<unidb::sql::ColumnDefinition> columns;
  columns.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    columns.push_back({column.name, fromRdbType(column.type)});
  }
  return columns;
}

void RdbServiceAdapter::insertRow(const std::string &tableName,
                                  const std::vector<char> &tupleBytes) {
  (void)insertRowAction.execute(metadataPageId, tableName, tupleBytes);
}

std::vector<std::vector<char>>
RdbServiceAdapter::selectAllRows(const std::string &tableName) {
  return selectAllRowsAction.execute(metadataPageId, tableName);
}

} // namespace QueryExecutionService
