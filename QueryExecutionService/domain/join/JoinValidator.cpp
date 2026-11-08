#include "JoinValidator.h"

#include "../schema/SqlSchemaResolver.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace QueryExecutionService::domain {
using unidb::sql::ColumnDefinition;
using unidb::sql::SelectStatement;

void validateInnerJoinColumns(
    const SelectStatement &select,
    const std::vector<ColumnDefinition> &leftSchema,
    const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = select.innerJoins.front();
  const std::string leftQualifier =
      select.tableAlias.value_or(select.tableName);
  const std::string rightQualifier = join.tableAlias.value_or(join.tableName);
  if (leftQualifier == rightQualifier) {
    throw std::invalid_argument("JOIN table aliases must be distinct");
  }
  if (select.tableName == join.tableName &&
      (!select.tableAlias.has_value() || !join.tableAlias.has_value())) {
    throw std::invalid_argument(
        "Joining a table to itself requires distinct table aliases");
  }

  const auto leftKey = resolveJoinColumn(join.left, leftQualifier, leftSchema,
                                         rightQualifier, rightSchema);
  const auto rightKey = resolveJoinColumn(join.right, leftQualifier, leftSchema,
                                          rightQualifier, rightSchema);
  if (leftKey.tableName == rightKey.tableName) {
    throw std::invalid_argument(
        "INNER JOIN ON must compare columns from both tables");
  }
  if (leftKey.column->type != rightKey.column->type) {
    throw std::invalid_argument("INNER JOIN columns must have matching types");
  }

  if (select.aggregate.has_value() || !select.groupBy.empty() ||
      select.having.has_value() || select.distinct || !select.orderBy.empty() ||
      select.limit.has_value() || select.offset != 0) {
    throw std::invalid_argument(
        "JOIN currently supports only a simple SELECT projection");
  }

  std::vector<std::string> outputNames;
  if (select.selectAll) {
    outputNames.reserve(leftSchema.size() + rightSchema.size());
    for (const auto &column : leftSchema) {
      const bool duplicated = std::any_of(
          rightSchema.begin(), rightSchema.end(),
          [&](const auto &right) { return right.name == column.name; });
      outputNames.push_back(duplicated ? leftQualifier + "." + column.name
                                       : column.name);
    }
    for (const auto &column : rightSchema) {
      const bool duplicated = std::any_of(
          leftSchema.begin(), leftSchema.end(),
          [&](const auto &left) { return left.name == column.name; });
      outputNames.push_back(duplicated ? rightQualifier + "." + column.name
                                       : column.name);
    }
  } else {
    if (select.columnNames.empty()) {
      throw std::invalid_argument("SELECT requires at least one column");
    }
    if (!select.columnAliases.empty() &&
        select.columnAliases.size() != select.columnNames.size()) {
      throw std::invalid_argument("SELECT alias count does not match columns");
    }
    outputNames.reserve(select.columnNames.size());
    for (std::size_t index = 0; index < select.columnNames.size(); ++index) {
      const auto resolved =
          resolveSelectedJoinColumn(select.columnNames[index], leftQualifier,
                                    leftSchema, rightQualifier, rightSchema);
      const bool duplicated =
          std::any_of(leftSchema.begin(), leftSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      }) &&
          std::any_of(rightSchema.begin(), rightSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      });
      std::string outputName =
          duplicated ? resolved.tableName + "." + resolved.column->name
                     : resolved.column->name;
      if (index < select.columnAliases.size() &&
          select.columnAliases[index].has_value()) {
        outputName = *select.columnAliases[index];
      }
      outputNames.push_back(std::move(outputName));
    }
  }

  std::unordered_set<std::string> uniqueOutputNames;
  for (const auto &name : outputNames) {
    if (!uniqueOutputNames.insert(name).second) {
      throw std::invalid_argument("Duplicate SELECT output column name: " +
                                  name);
    }
  }
}

} // namespace QueryExecutionService::domain
