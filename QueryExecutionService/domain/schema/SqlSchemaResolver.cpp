#include "SqlSchemaResolver.h"

#include <algorithm>
#include <stdexcept>

namespace QueryExecutionService::domain {
using unidb::sql::ColumnDefinition;
using unidb::sql::ColumnReference;
using unidb::sql::SqlType;

std::vector<SqlType> schemaTypes(const std::vector<ColumnDefinition> &schema) {
  std::vector<SqlType> types;
  types.reserve(schema.size());
  for (const auto &column : schema) {
    if (column.declaredType.has_value()) {
      using unidb::sql::SqlTypeId;
      switch (column.declaredType->id) {
      case SqlTypeId::SmallInt:
      case SqlTypeId::Integer:
      case SqlTypeId::BigInt:
      case SqlTypeId::Real:
      case SqlTypeId::DoublePrecision:
      case SqlTypeId::Boolean:
      case SqlTypeId::Numeric:
      case SqlTypeId::Date:
      case SqlTypeId::Time:
      case SqlTypeId::TimeWithTimeZone:
      case SqlTypeId::Timestamp:
      case SqlTypeId::TimestampWithTimeZone:
      case SqlTypeId::Interval:
        // Date/time values are not executable yet. Keep the legacy physical
        // type tag only so schema discovery can expose their canonical
        // declaredType metadata without rejecting SELECT *.
        types.push_back(SqlType::Int);
        continue;
      case SqlTypeId::Char:
      case SqlTypeId::Varchar:
      case SqlTypeId::Text:
        types.push_back(SqlType::Text);
        continue;
      default:
        throw std::invalid_argument(
            "Column type is outside the currently supported SQL subset");
      }
    }
    if (column.type != SqlType::Int && column.type != SqlType::Text) {
      throw std::invalid_argument("Column type is outside the V1 SQL subset");
    }
    types.push_back(column.type);
  }
  return types;
}

ResolvedJoinColumn
resolveJoinColumn(const unidb::sql::ColumnReference &reference,
                  const std::string &leftTableName,
                  const std::vector<ColumnDefinition> &leftSchema,
                  const std::string &rightTableName,
                  const std::vector<ColumnDefinition> &rightSchema) {
  if (!reference.tableName.has_value()) {
    throw std::invalid_argument(
        "JOIN ON requires table-qualified column references");
  }

  const std::vector<ColumnDefinition> *schema = nullptr;
  if (*reference.tableName == leftTableName) {
    schema = &leftSchema;
  } else if (*reference.tableName == rightTableName) {
    schema = &rightSchema;
  } else {
    throw std::invalid_argument("Unknown JOIN table reference: " +
                                *reference.tableName);
  }

  const auto found =
      std::find_if(schema->begin(), schema->end(), [&](const auto &column) {
        return column.name == reference.columnName;
      });
  if (found == schema->end()) {
    throw std::invalid_argument("Unknown JOIN column: " + *reference.tableName +
                                "." + reference.columnName);
  }

  return {&*found, *reference.tableName,
          static_cast<std::size_t>(std::distance(schema->begin(), found))};
}

ResolvedJoinColumn
resolveSelectedJoinColumn(const std::string &name,
                          const std::string &leftTableName,
                          const std::vector<ColumnDefinition> &leftSchema,
                          const std::string &rightTableName,
                          const std::vector<ColumnDefinition> &rightSchema) {
  const auto separator = name.find('.');
  if (separator != std::string::npos) {
    if (name.find('.', separator + 1) != std::string::npos) {
      throw std::invalid_argument("Invalid qualified SELECT column: " + name);
    }
    const std::string tableName = name.substr(0, separator);
    const std::string columnName = name.substr(separator + 1);
    if (tableName != leftTableName && tableName != rightTableName) {
      throw std::invalid_argument("Unknown SELECT table: " + tableName);
    }
    const auto &schema = tableName == leftTableName ? leftSchema : rightSchema;
    const auto found =
        std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
          return column.name == columnName;
        });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown SELECT column: " + name);
    }
    return {&*found, tableName,
            static_cast<std::size_t>(std::distance(schema.begin(), found))};
  }

  const auto leftFound =
      std::find_if(leftSchema.begin(), leftSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const auto rightFound =
      std::find_if(rightSchema.begin(), rightSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const bool inLeft = leftFound != leftSchema.end();
  const bool inRight = rightFound != rightSchema.end();

  if (!inLeft && !inRight) {
    throw std::invalid_argument("Unknown SELECT column: " + name);
  }
  if (inLeft && inRight) {
    throw std::invalid_argument("Ambiguous SELECT column: " + name);
  }
  if (inLeft) {
    return {
        &*leftFound, leftTableName,
        static_cast<std::size_t>(std::distance(leftSchema.begin(), leftFound))};
  }
  return {
      &*rightFound, rightTableName,
      static_cast<std::size_t>(std::distance(rightSchema.begin(), rightFound))};
}

ResolvedJoinColumn
resolveWhereJoinColumn(const std::string &name,
                       const std::string &leftTableName,
                       const std::vector<ColumnDefinition> &leftSchema,
                       const std::string &rightTableName,
                       const std::vector<ColumnDefinition> &rightSchema) {
  const auto separator = name.find('.');
  if (separator != std::string::npos) {
    return resolveSelectedJoinColumn(name, leftTableName, leftSchema,
                                     rightTableName, rightSchema);
  }
  const auto left =
      std::find_if(leftSchema.begin(), leftSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const auto right =
      std::find_if(rightSchema.begin(), rightSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const bool inLeft = left != leftSchema.end();
  const bool inRight = right != rightSchema.end();
  if (inLeft && inRight) {
    throw std::invalid_argument("Ambiguous WHERE column: " + name);
  }
  if (!inLeft && !inRight) {
    throw std::invalid_argument("Unknown WHERE column: " + name);
  }
  if (inLeft) {
    return {&*left, leftTableName,
            static_cast<std::size_t>(std::distance(leftSchema.begin(), left))};
  }
  return {&*right, rightTableName,
          static_cast<std::size_t>(std::distance(rightSchema.begin(), right))};
}

std::string normalizeSingleTableColumnName(const std::string &name,
                                           const std::string &tableQualifier) {
  const auto separator = name.find('.');
  if (separator == std::string::npos) {
    return name;
  }
  if (name.find('.', separator + 1) != std::string::npos ||
      name.substr(0, separator) != tableQualifier) {
    throw std::invalid_argument("Unknown SELECT table reference: " +
                                name.substr(0, separator));
  }
  return name.substr(separator + 1);
}

} // namespace QueryExecutionService::domain
