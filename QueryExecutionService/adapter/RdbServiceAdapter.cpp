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

RdbDataType toRdbType(const unidb::sql::ColumnDefinition &column) {
  if (!column.declaredType.has_value()) {
    return toRdbType(column.type);
  }

  switch (column.declaredType->id) {
  case unidb::sql::SqlTypeId::SmallInt:
    return RdbDataType::SMALLINT;
  case unidb::sql::SqlTypeId::Integer:
    return RdbDataType::INT;
  case unidb::sql::SqlTypeId::BigInt:
    return RdbDataType::BIGINT;
  case unidb::sql::SqlTypeId::Boolean:
    return RdbDataType::BOOLEAN;
  case unidb::sql::SqlTypeId::Text:
    return RdbDataType::VARCHAR;
  default:
    throw std::invalid_argument(
        "CREATE TABLE type is recognized but not yet supported by storage");
  }
}

unidb::sql::SqlType fromRdbType(RdbDataType type) {
  switch (type) {
  case RdbDataType::INT:
  case RdbDataType::SMALLINT:
  case RdbDataType::BIGINT:
    return unidb::sql::SqlType::Int;
  case RdbDataType::BOOLEAN:
    // The declared descriptor is authoritative until BOOLEAN query planning
    // adopts a dedicated transitional execution tag.
    return unidb::sql::SqlType::Int;
  case RdbDataType::VARCHAR:
    return unidb::sql::SqlType::Text;
  default:
    throw std::invalid_argument(
        "Table schema contains a type outside the V1 SQL subset");
  }
}

unidb::sql::SqlTypeDescriptor descriptorFromRdbType(RdbDataType type) {
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;
  switch (type) {
  case RdbDataType::SMALLINT:
    return SqlTypeDescriptor{SqlTypeId::SmallInt, {}};
  case RdbDataType::INT:
    return SqlTypeDescriptor{SqlTypeId::Integer, {}};
  case RdbDataType::BIGINT:
    return SqlTypeDescriptor{SqlTypeId::BigInt, {}};
  case RdbDataType::BOOLEAN:
    return SqlTypeDescriptor{SqlTypeId::Boolean, {}};
  case RdbDataType::VARCHAR:
    return SqlTypeDescriptor{SqlTypeId::Text, {}};
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
    const auto type = toRdbType(column);
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
    unidb::sql::ColumnDefinition definition{column.name,
                                            fromRdbType(column.type)};
    definition.declaredType = descriptorFromRdbType(column.type);
    columns.push_back(std::move(definition));
  }
  return columns;
}

void RdbServiceAdapter::insertRow(const std::string &tableName,
                                  const std::vector<char> &tupleBytes) {
  try {
    (void)insertRowAction.execute(metadataPageId, tableName, tupleBytes);
  } catch (const RdbService::SpaceMapUpdateFailedAfterInsert &error) {
    throw RowInsertedButPostInsertUpdateFailed(error.what());
  }
}

std::vector<std::vector<char>>
RdbServiceAdapter::selectAllRows(const std::string &tableName) {
  return selectAllRowsAction.execute(metadataPageId, tableName);
}

} // namespace QueryExecutionService
