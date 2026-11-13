#include "JoinValidator.h"

#include <algorithm>

#include "JoinProjectionPlanner.h"

#include "../schema/SqlSchemaResolver.h"

#include <stdexcept>
#include <string>

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

  (void)buildJoinProjectionPlan(select, {leftSchema, rightSchema},
                                {leftQualifier, rightQualifier});
}

void validateLeftJoinColumns(const SelectStatement &select,
                             const std::vector<ColumnDefinition> &leftSchema,
                             const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = *select.leftJoin;
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
        "LEFT JOIN ON must compare columns from both tables");
  }
  if (leftKey.column->type != rightKey.column->type) {
    throw std::invalid_argument("LEFT JOIN columns must have matching types");
  }
  if (select.aggregate.has_value() || !select.groupBy.empty() ||
      select.having.has_value() || select.distinct || !select.orderBy.empty() ||
      select.limit.has_value() || select.offset != 0 ||
      !std::all_of(select.integerExpressions.begin(),
                   select.integerExpressions.end(),
                   [](const auto &value) { return !value.has_value(); }) ||
      !std::all_of(select.textFunctions.begin(), select.textFunctions.end(),
                   [](const auto &value) { return !value.has_value(); }) ||
      !std::all_of(select.caseExpressions.begin(), select.caseExpressions.end(),
                   [](const auto &value) { return !value.has_value(); })) {
    throw std::invalid_argument(
        "LEFT JOIN currently supports simple SELECT projections and WHERE");
  }
  (void)buildJoinProjectionPlan(select, {leftSchema, rightSchema},
                                {leftQualifier, rightQualifier});
}

} // namespace QueryExecutionService::domain
