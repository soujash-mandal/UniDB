#include "RdbServiceAdapter.h"

#include "../../RdbService/domain/RdbTable.h"

#include <limits>
#include <optional>
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
  case unidb::sql::SqlTypeId::Real:
    return RdbDataType::FLOAT;
  case unidb::sql::SqlTypeId::DoublePrecision:
    return RdbDataType::DOUBLE;
  case unidb::sql::SqlTypeId::Boolean:
    return RdbDataType::BOOLEAN;
  case unidb::sql::SqlTypeId::Char:
    return RdbDataType::CHAR;
  case unidb::sql::SqlTypeId::Varchar:
    return RdbDataType::VARCHAR;
  case unidb::sql::SqlTypeId::Text:
    return RdbDataType::TEXT;
  case unidb::sql::SqlTypeId::Numeric:
    return RdbDataType::NUMERIC;
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
  case RdbDataType::FLOAT:
  case RdbDataType::DOUBLE:
    // The declared descriptor remains authoritative until floating-point
    // query execution adopts dedicated transitional execution tags.
    return unidb::sql::SqlType::Int;
  case RdbDataType::NUMERIC:
    // The declared descriptor remains authoritative for exact decimals.
    return unidb::sql::SqlType::Int;
  case RdbDataType::BOOLEAN:
    // The declared descriptor is authoritative until BOOLEAN query planning
    // adopts a dedicated transitional execution tag.
    return unidb::sql::SqlType::Int;
  case RdbDataType::VARCHAR:
  case RdbDataType::CHAR:
  case RdbDataType::TEXT:
    return unidb::sql::SqlType::Text;
  default:
    throw std::invalid_argument(
        "Table schema contains a type outside the V1 SQL subset");
  }
}

unidb::sql::SqlTypeDescriptor descriptorFromRdbType(const RdbColumn &column) {
  const RdbDataType type = column.type;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;
  switch (type) {
  case RdbDataType::SMALLINT:
    return SqlTypeDescriptor{SqlTypeId::SmallInt, {}};
  case RdbDataType::INT:
    return SqlTypeDescriptor{SqlTypeId::Integer, {}};
  case RdbDataType::BIGINT:
    return SqlTypeDescriptor{SqlTypeId::BigInt, {}};
  case RdbDataType::FLOAT:
    return SqlTypeDescriptor{SqlTypeId::Real, {}};
  case RdbDataType::DOUBLE:
    return SqlTypeDescriptor{SqlTypeId::DoublePrecision, {}};
  case RdbDataType::BOOLEAN:
    return SqlTypeDescriptor{SqlTypeId::Boolean, {}};
  case RdbDataType::CHAR: {
    unidb::sql::SqlTypeModifier modifier;
    modifier.length = column.characterLength;
    return SqlTypeDescriptor{SqlTypeId::Char, modifier};
  }
  case RdbDataType::VARCHAR: {
    unidb::sql::SqlTypeModifier modifier;
    modifier.length = column.characterLength;
    return SqlTypeDescriptor{SqlTypeId::Varchar, modifier};
  }
  case RdbDataType::TEXT:
    return SqlTypeDescriptor{SqlTypeId::Text, {}};
  case RdbDataType::NUMERIC: {
    unidb::sql::SqlTypeModifier modifier;
    modifier.precision = column.numericPrecision;
    modifier.scale = column.numericScale;
    return SqlTypeDescriptor{SqlTypeId::Numeric, modifier};
  }
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
        (type == RdbDataType::VARCHAR || type == RdbDataType::CHAR ||
         type == RdbDataType::TEXT)
            ? std::numeric_limits<uint16_t>::max()
            : 0;
    std::optional<std::uint32_t> numericPrecision;
    std::optional<std::int32_t> numericScale;
    if (column.declaredType.has_value() &&
        column.declaredType->id == unidb::sql::SqlTypeId::Numeric) {
      numericPrecision = column.declaredType->modifier.precision;
      numericScale = column.declaredType->modifier.scale;
    }
    std::optional<std::uint32_t> characterLength;
    if (column.declaredType.has_value() &&
        (column.declaredType->id == unidb::sql::SqlTypeId::Char ||
         column.declaredType->id == unidb::sql::SqlTypeId::Varchar)) {
      characterLength = column.declaredType->modifier.length;
    }
    table.columns.push_back(RdbColumn{column.name, type, size, false,
                                      numericPrecision, numericScale,
                                      characterLength});
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
    definition.declaredType = descriptorFromRdbType(column);
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
