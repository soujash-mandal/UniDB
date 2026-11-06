#include "ExecuteQueryAction.h"

#include "../adapter/SqlTupleCodec.h"
#include "../domain/expression/ScalarExpressionEvaluator.h"
#include "../domain/predicate/WherePredicateEvaluator.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService {
namespace {

using unidb::sql::AggregateFunction;
using unidb::sql::ColumnDefinition;
using unidb::sql::ComparisonOperator;
using unidb::sql::CreateTableStatement;
using unidb::sql::HavingExpression;
using unidb::sql::InsertStatement;
using unidb::sql::IntegerBinaryOperator;
using unidb::sql::IntegerExpression;
using unidb::sql::IntegerUnaryOperator;
using unidb::sql::SelectStatement;
using unidb::sql::SqlStatement;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;
using unidb::sql::TextFunction;
using unidb::sql::TextFunctionExpression;
using unidb::sql::UnionAllStatement;
using unidb::sql::UnionStatement;
using unidb::sql::WhereEquals;
using unidb::sql::WhereExpression;

QueryExecutionError executionError(std::string message) {
  return {QueryExecutionErrorCode::ExecutionError, std::move(message)};
}

std::vector<SqlType> schemaTypes(const std::vector<ColumnDefinition> &schema) {
  std::vector<SqlType> types;
  types.reserve(schema.size());
  for (const auto &column : schema) {
    if (column.type != SqlType::Int && column.type != SqlType::Text) {
      throw std::invalid_argument("Column type is outside the V1 SQL subset");
    }
    types.push_back(column.type);
  }
  return types;
}

struct ResolvedJoinColumn {
  const ColumnDefinition *column;
  std::string tableName;
  std::size_t index;
};

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

// Execute INNER JOIN entirely in QES using the existing public row-read port.
// Both inputs and the joined result are in memory; matching is O(N x M).
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

domain::CompiledTextFunction
compileTextFunction(const TextFunctionExpression &expression,
                    const std::vector<ColumnDefinition> &schema,
                    const std::vector<SqlType> &types,
                    const std::string &tableQualifier) {
  const auto name =
      normalizeSingleTableColumnName(expression.columnName, tableQualifier);
  const auto found =
      std::find_if(schema.begin(), schema.end(),
                   [&](const auto &column) { return column.name == name; });
  if (found == schema.end()) {
    throw std::invalid_argument("Unknown SELECT column: " +
                                expression.columnName);
  }
  const auto index =
      static_cast<std::size_t>(std::distance(schema.begin(), found));
  if (types[index] != SqlType::Text) {
    throw std::invalid_argument("Scalar text functions require TEXT columns: " +
                                expression.columnName);
  }
  return {expression.function, index};
}

std::string textFunctionName(TextFunction function) {
  switch (function) {
  case TextFunction::Upper:
    return "UPPER";
  case TextFunction::Lower:
    return "LOWER";
  case TextFunction::Length:
    return "LENGTH";
  }
  throw std::invalid_argument("Unknown scalar text function");
}

domain::CompiledIntegerExpression
compileIntegerExpression(const IntegerExpression &expression,
                         const std::vector<ColumnDefinition> &schema,
                         const std::vector<SqlType> &types,
                         const std::string &tableQualifier) {
  domain::CompiledIntegerExpression compiled;
  compiled.kind = expression.kind;
  if (expression.kind == IntegerExpression::Kind::Literal) {
    compiled.literal = expression.literal;
    return compiled;
  }
  if (expression.kind == IntegerExpression::Kind::Column) {
    const auto name =
        normalizeSingleTableColumnName(expression.columnName, tableQualifier);
    const auto found =
        std::find_if(schema.begin(), schema.end(),
                     [&](const auto &column) { return column.name == name; });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown SELECT column: " +
                                  expression.columnName);
    }
    const auto index =
        static_cast<std::size_t>(std::distance(schema.begin(), found));
    if (types[index] != SqlType::Int) {
      throw std::invalid_argument("Integer arithmetic requires INT columns: " +
                                  expression.columnName);
    }
    compiled.columnIndex = index;
    return compiled;
  }
  if (expression.kind == IntegerExpression::Kind::Unary) {
    if (!expression.operand) {
      throw std::invalid_argument("Malformed unary integer expression");
    }
    compiled.unaryOperation = expression.unaryOperation;
    compiled.operand = std::make_shared<domain::CompiledIntegerExpression>(
        compileIntegerExpression(*expression.operand, schema, types,
                                 tableQualifier));
    return compiled;
  }
  if (!expression.left || !expression.right) {
    throw std::invalid_argument("Malformed integer arithmetic expression");
  }
  compiled.operation = expression.operation;
  compiled.left = std::make_shared<domain::CompiledIntegerExpression>(
      compileIntegerExpression(*expression.left, schema, types,
                               tableQualifier));
  compiled.right = std::make_shared<domain::CompiledIntegerExpression>(
      compileIntegerExpression(*expression.right, schema, types,
                               tableQualifier));
  return compiled;
}

WhereExpression
normalizeSingleTableWhereColumns(const WhereExpression &expression,
                                 const std::string &tableName) {
  WhereExpression normalized = expression;
  if (normalized.kind == WhereExpression::Kind::Predicate) {
    const auto separator = normalized.predicate.columnName.find('.');
    if (separator != std::string::npos) {
      const std::string qualifier =
          normalized.predicate.columnName.substr(0, separator);
      if (qualifier != tableName ||
          normalized.predicate.columnName.find('.', separator + 1) !=
              std::string::npos) {
        throw std::invalid_argument("Unknown WHERE table reference: " +
                                    qualifier);
      }
      normalized.predicate.columnName =
          normalized.predicate.columnName.substr(separator + 1);
    }
    return normalized;
  }
  for (auto &child : normalized.children) {
    child = normalizeSingleTableWhereColumns(child, tableName);
  }
  return normalized;
}

WhereExpression
qualifyJoinWhereColumns(const WhereExpression &expression,
                        const std::string &leftTableName,
                        const std::vector<ColumnDefinition> &leftSchema,
                        const std::string &rightTableName,
                        const std::vector<ColumnDefinition> &rightSchema) {
  WhereExpression qualified = expression;
  if (qualified.kind == WhereExpression::Kind::Predicate) {
    const auto resolved =
        resolveWhereJoinColumn(qualified.predicate.columnName, leftTableName,
                               leftSchema, rightTableName, rightSchema);
    qualified.predicate.columnName =
        resolved.tableName + "." + resolved.column->name;
    return qualified;
  }
  for (auto &child : qualified.children) {
    child = qualifyJoinWhereColumns(child, leftTableName, leftSchema,
                                    rightTableName, rightSchema);
  }
  return qualified;
}

QueryExecutionResult
executeInnerJoin(RdbServicePort &rdbServicePort, const SelectStatement &select,
                 const std::vector<ColumnDefinition> &leftSchema,
                 const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = select.innerJoins.front();
  const std::string leftQualifier =
      select.tableAlias.value_or(select.tableName);
  const std::string rightQualifier = join.tableAlias.value_or(join.tableName);
  const auto leftTypes = schemaTypes(leftSchema);
  const auto rightTypes = schemaTypes(rightSchema);
  const auto leftKey = resolveJoinColumn(join.left, leftQualifier, leftSchema,
                                         rightQualifier, rightSchema);
  const auto rightKey = resolveJoinColumn(join.right, leftQualifier, leftSchema,
                                          rightQualifier, rightSchema);
  const std::size_t leftKeyIndex =
      leftKey.tableName == leftQualifier ? leftKey.index : rightKey.index;
  const std::size_t rightKeyIndex =
      leftKey.tableName == rightQualifier ? leftKey.index : rightKey.index;

  // Use fully-qualified internal names so WHERE can resolve either qualified
  // references or unqualified names after ambiguity checks.
  std::vector<ColumnDefinition> joinedSchema;
  std::vector<SqlType> joinedTypes;
  joinedSchema.reserve(leftSchema.size() + rightSchema.size());
  joinedTypes.reserve(leftTypes.size() + rightTypes.size());
  for (const auto &column : leftSchema) {
    joinedSchema.push_back({leftQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }
  for (const auto &column : rightSchema) {
    joinedSchema.push_back({rightQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }

  std::optional<domain::CompiledWhereExpression> compiledWhere;
  if (const auto *where = std::get_if<WhereExpression>(&select.where)) {
    const auto qualified = qualifyJoinWhereColumns(
        *where, leftQualifier, leftSchema, rightQualifier, rightSchema);
    compiledWhere =
        domain::compileWhereExpression(qualified, joinedSchema, joinedTypes);
  }

  std::vector<std::size_t> selectedIndexes;
  std::vector<ColumnDefinition> selectedColumns;
  if (select.selectAll) {
    selectedIndexes.reserve(joinedSchema.size());
    selectedColumns.reserve(joinedSchema.size());
    for (std::size_t index = 0; index < joinedSchema.size(); ++index) {
      selectedIndexes.push_back(index);
      const bool isLeft = index < leftSchema.size();
      const auto &sourceColumn =
          isLeft ? leftSchema[index] : rightSchema[index - leftSchema.size()];
      const auto &otherSchema = isLeft ? rightSchema : leftSchema;
      const bool duplicated = std::any_of(
          otherSchema.begin(), otherSchema.end(),
          [&](const auto &other) { return other.name == sourceColumn.name; });
      const std::string &tableName = isLeft ? leftQualifier : rightQualifier;
      selectedColumns.push_back(
          {duplicated ? tableName + "." + sourceColumn.name : sourceColumn.name,
           sourceColumn.type});
    }
  } else {
    selectedIndexes.reserve(select.columnNames.size());
    selectedColumns.reserve(select.columnNames.size());
    for (std::size_t index = 0; index < select.columnNames.size(); ++index) {
      const auto resolved =
          resolveSelectedJoinColumn(select.columnNames[index], leftQualifier,
                                    leftSchema, rightQualifier, rightSchema);
      selectedIndexes.push_back(resolved.tableName == leftQualifier
                                    ? resolved.index
                                    : leftSchema.size() + resolved.index);
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
      selectedColumns.push_back({std::move(outputName), resolved.column->type});
    }
  }

  const auto leftEncodedRows = rdbServicePort.selectAllRows(select.tableName);
  const auto rightEncodedRows = rdbServicePort.selectAllRows(join.tableName);
  std::vector<std::vector<SqlValue>> leftRows;
  leftRows.reserve(leftEncodedRows.size());
  for (const auto &bytes : leftEncodedRows) {
    leftRows.push_back(SqlTupleCodec::decode(leftTypes, bytes));
  }
  std::vector<std::vector<SqlValue>> rightRows;
  rightRows.reserve(rightEncodedRows.size());
  for (const auto &bytes : rightEncodedRows) {
    rightRows.push_back(SqlTupleCodec::decode(rightTypes, bytes));
  }

  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = std::move(selectedColumns);
  for (const auto &leftRow : leftRows) {
    for (const auto &rightRow : rightRows) {
      const int comparison = domain::compareSqlValues(leftRow[leftKeyIndex],
                                                      rightRow[rightKeyIndex]);
      bool matches = false;
      switch (join.comparison) {
      case ComparisonOperator::Equal:
        matches = comparison == 0;
        break;
      case ComparisonOperator::NotEqual:
        matches = comparison != 0;
        break;
      case ComparisonOperator::GreaterThan:
        matches = comparison > 0;
        break;
      case ComparisonOperator::LessThan:
        matches = comparison < 0;
        break;
      case ComparisonOperator::GreaterThanOrEqual:
        matches = comparison >= 0;
        break;
      case ComparisonOperator::LessThanOrEqual:
        matches = comparison <= 0;
        break;
      default:
        throw std::invalid_argument(
            "INNER JOIN supports only basic comparisons");
      }
      if (!matches) {
        continue;
      }
      std::vector<SqlValue> joinedRow;
      joinedRow.reserve(leftRow.size() + rightRow.size());
      joinedRow.insert(joinedRow.end(), leftRow.begin(), leftRow.end());
      joinedRow.insert(joinedRow.end(), rightRow.begin(), rightRow.end());
      if (compiledWhere.has_value() &&
          !domain::matchesWhereExpression(joinedRow, *compiledWhere)) {
        continue;
      }
      std::vector<SqlValue> projectedRow;
      projectedRow.reserve(selectedIndexes.size());
      for (const auto index : selectedIndexes) {
        projectedRow.push_back(joinedRow[index]);
      }
      result.rows.push_back(std::move(projectedRow));
    }
  }
  return result;
}

QueryExecutionResult
executeCrossJoin(RdbServicePort &rdbServicePort, const SelectStatement &select,
                 const std::vector<ColumnDefinition> &leftSchema,
                 const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = *select.crossJoin;
  const std::string leftQualifier =
      select.tableAlias.value_or(select.tableName);
  const std::string rightQualifier = join.tableAlias.value_or(join.tableName);
  if (leftQualifier == rightQualifier ||
      (select.tableName == join.tableName && !select.tableAlias &&
       !join.tableAlias)) {
    throw std::invalid_argument(
        "CROSS JOIN of the same table requires distinct aliases");
  }
  if (select.aggregate || !select.groupBy.empty() || select.having ||
      select.distinct || !select.orderBy.empty() || select.limit ||
      select.offset != 0 ||
      !std::all_of(select.integerExpressions.begin(),
                   select.integerExpressions.end(),
                   [](const auto &value) { return !value.has_value(); }) ||
      !std::all_of(select.textFunctions.begin(), select.textFunctions.end(),
                   [](const auto &value) { return !value.has_value(); }) ||
      !std::all_of(select.caseExpressions.begin(), select.caseExpressions.end(),
                   [](const auto &value) { return !value.has_value(); })) {
    throw std::invalid_argument(
        "CROSS JOIN currently supports simple SELECT projections and WHERE");
  }

  const auto leftTypes = schemaTypes(leftSchema);
  const auto rightTypes = schemaTypes(rightSchema);
  // Use fully-qualified internal names so WHERE can resolve either qualified
  // references or unqualified names after ambiguity checks.
  std::vector<ColumnDefinition> joinedSchema;
  std::vector<SqlType> joinedTypes;
  joinedSchema.reserve(leftSchema.size() + rightSchema.size());
  joinedTypes.reserve(leftTypes.size() + rightTypes.size());
  for (const auto &column : leftSchema) {
    joinedSchema.push_back({leftQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }
  for (const auto &column : rightSchema) {
    joinedSchema.push_back({rightQualifier + "." + column.name, column.type});
    joinedTypes.push_back(column.type);
  }

  std::optional<domain::CompiledWhereExpression> compiledWhere;
  if (const auto *where = std::get_if<WhereExpression>(&select.where)) {
    const auto qualified = qualifyJoinWhereColumns(
        *where, leftQualifier, leftSchema, rightQualifier, rightSchema);
    compiledWhere =
        domain::compileWhereExpression(qualified, joinedSchema, joinedTypes);
  }

  std::vector<std::size_t> selectedIndexes;
  std::vector<ColumnDefinition> selectedColumns;
  if (select.selectAll) {
    selectedIndexes.reserve(joinedSchema.size());
    selectedColumns.reserve(joinedSchema.size());
    for (std::size_t index = 0; index < joinedSchema.size(); ++index) {
      selectedIndexes.push_back(index);
      const bool isLeft = index < leftSchema.size();
      const auto &sourceColumn =
          isLeft ? leftSchema[index] : rightSchema[index - leftSchema.size()];
      const auto &otherSchema = isLeft ? rightSchema : leftSchema;
      const bool duplicated = std::any_of(
          otherSchema.begin(), otherSchema.end(),
          [&](const auto &other) { return other.name == sourceColumn.name; });
      const std::string &tableName = isLeft ? leftQualifier : rightQualifier;
      selectedColumns.push_back(
          {duplicated ? tableName + "." + sourceColumn.name : sourceColumn.name,
           sourceColumn.type});
    }
  } else {
    selectedIndexes.reserve(select.columnNames.size());
    selectedColumns.reserve(select.columnNames.size());
    for (std::size_t index = 0; index < select.columnNames.size(); ++index) {
      const auto resolved =
          resolveSelectedJoinColumn(select.columnNames[index], leftQualifier,
                                    leftSchema, rightQualifier, rightSchema);
      selectedIndexes.push_back(resolved.tableName == leftQualifier
                                    ? resolved.index
                                    : leftSchema.size() + resolved.index);
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
      selectedColumns.push_back({std::move(outputName), resolved.column->type});
    }
  }

  const auto leftEncodedRows = rdbServicePort.selectAllRows(select.tableName);
  const auto rightEncodedRows = rdbServicePort.selectAllRows(join.tableName);
  std::vector<std::vector<SqlValue>> leftRows;
  leftRows.reserve(leftEncodedRows.size());
  for (const auto &bytes : leftEncodedRows) {
    leftRows.push_back(SqlTupleCodec::decode(leftTypes, bytes));
  }
  std::vector<std::vector<SqlValue>> rightRows;
  rightRows.reserve(rightEncodedRows.size());
  for (const auto &bytes : rightEncodedRows) {
    rightRows.push_back(SqlTupleCodec::decode(rightTypes, bytes));
  }
  constexpr std::size_t kMaxCrossJoinRows = 1'000'000;
  if (!leftRows.empty() &&
      rightRows.size() > kMaxCrossJoinRows / leftRows.size()) {
    throw std::length_error(
        "CROSS JOIN result exceeds the 1000000-row safety limit");
  }

  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = std::move(selectedColumns);
  for (const auto &leftRow : leftRows) {
    for (const auto &rightRow : rightRows) {
      std::vector<SqlValue> joinedRow;
      joinedRow.reserve(leftRow.size() + rightRow.size());
      joinedRow.insert(joinedRow.end(), leftRow.begin(), leftRow.end());
      joinedRow.insert(joinedRow.end(), rightRow.begin(), rightRow.end());
      if (compiledWhere.has_value() &&
          !domain::matchesWhereExpression(joinedRow, *compiledWhere)) {
        continue;
      }
      std::vector<SqlValue> projectedRow;
      projectedRow.reserve(selectedIndexes.size());
      for (const auto index : selectedIndexes) {
        projectedRow.push_back(joinedRow[index]);
      }
      result.rows.push_back(std::move(projectedRow));
    }
  }
  return result;
}

QueryExecutionResult executeMultipleInnerJoins(
    RdbServicePort &port, const SelectStatement &select,
    const std::vector<std::vector<ColumnDefinition>> &schemas) {
  if (select.innerJoins.size() < 2 ||
      schemas.size() != select.innerJoins.size() + 1)
    throw std::invalid_argument("Invalid multiple INNER JOIN plan");
  if (select.aggregate || !select.groupBy.empty() || select.having ||
      select.distinct || !select.orderBy.empty() || select.limit ||
      select.offset != 0 ||
      !std::all_of(select.integerExpressions.begin(),
                   select.integerExpressions.end(),
                   [](const auto &v) { return !v.has_value(); }) ||
      !std::all_of(select.textFunctions.begin(), select.textFunctions.end(),
                   [](const auto &v) { return !v.has_value(); }) ||
      !std::all_of(select.caseExpressions.begin(), select.caseExpressions.end(),
                   [](const auto &v) { return !v.has_value(); }))
    throw std::invalid_argument(
        "Multiple INNER JOIN currently supports simple SELECT projections");
  std::vector<std::string> qualifiers;
  qualifiers.push_back(select.tableAlias.value_or(select.tableName));
  std::vector<ColumnDefinition> combinedSchema;
  std::vector<SqlType> combinedTypes;
  std::vector<std::vector<SqlValue>> rows;
  auto firstTypes = schemaTypes(schemas.front());
  for (const auto &c : schemas.front()) {
    combinedSchema.push_back({qualifiers.front() + "." + c.name, c.type});
    combinedTypes.push_back(c.type);
  }
  for (const auto &bytes : port.selectAllRows(select.tableName))
    rows.push_back(SqlTupleCodec::decode(firstTypes, bytes));
  for (std::size_t j = 0; j < select.innerJoins.size(); ++j) {
    const auto &join = select.innerJoins[j];
    const auto qualifier = join.tableAlias.value_or(join.tableName);
    if (std::find(qualifiers.begin(), qualifiers.end(), qualifier) !=
        qualifiers.end())
      throw std::invalid_argument("JOIN table aliases must be distinct");
    if (std::find_if(qualifiers.begin(), qualifiers.end(),
                     [&](const auto &q) { return q == join.tableName; }) !=
            qualifiers.end() &&
        !join.tableAlias)
      throw std::invalid_argument(
          "Joining a table to itself requires a distinct alias");
    auto resolve =
        [&](const unidb::sql::ColumnReference &ref,
            bool allowNew) -> std::pair<std::size_t, const ColumnDefinition *> {
      if (!ref.tableName)
        throw std::invalid_argument(
            "JOIN ON requires table-qualified column references");
      if (*ref.tableName == qualifier && allowNew) {
        const auto &sc = schemas[j + 1];
        auto it = std::find_if(sc.begin(), sc.end(), [&](const auto &c) {
          return c.name == ref.columnName;
        });
        if (it == sc.end())
          throw std::invalid_argument("Unknown JOIN column: " + *ref.tableName +
                                      "." + ref.columnName);
        return {static_cast<std::size_t>(std::distance(sc.begin(), it)), &*it};
      }
      auto q = std::find(qualifiers.begin(), qualifiers.end(), *ref.tableName);
      if (q == qualifiers.end())
        throw std::invalid_argument("Unknown JOIN table reference: " +
                                    *ref.tableName);
      const auto qi =
          static_cast<std::size_t>(std::distance(qualifiers.begin(), q));
      std::size_t offset = 0;
      for (std::size_t k = 0; k < qi; ++k)
        offset += schemas[k].size();
      const auto &sc = schemas[qi];
      auto it = std::find_if(sc.begin(), sc.end(), [&](const auto &c) {
        return c.name == ref.columnName;
      });
      if (it == sc.end())
        throw std::invalid_argument("Unknown JOIN column: " + *ref.tableName +
                                    "." + ref.columnName);
      return {offset + static_cast<std::size_t>(std::distance(sc.begin(), it)),
              &*it};
    };
    auto l = resolve(join.left, true), rr = resolve(join.right, true);
    const bool lnew = join.left.tableName && *join.left.tableName == qualifier;
    const bool rnew =
        join.right.tableName && *join.right.tableName == qualifier;
    if (lnew == rnew)
      throw std::invalid_argument("INNER JOIN ON must compare a previously "
                                  "joined table with the new table");
    if (l.second->type != rr.second->type)
      throw std::invalid_argument(
          "INNER JOIN columns must have matching types");
    const std::size_t priorIndex = lnew ? rr.first : l.first;
    const std::size_t newIndex = lnew ? l.first : rr.first;
    const auto newTypes = schemaTypes(schemas[j + 1]);
    std::vector<std::vector<SqlValue>> newRows;
    for (const auto &bytes : port.selectAllRows(join.tableName))
      newRows.push_back(SqlTupleCodec::decode(newTypes, bytes));
    std::vector<std::vector<SqlValue>> expanded;
    for (const auto &oldRow : rows)
      for (const auto &newRow : newRows) {
        const auto cmp = lnew ? domain::compareSqlValues(newRow[newIndex],
                                                         oldRow[priorIndex])
                              : domain::compareSqlValues(oldRow[priorIndex],
                                                         newRow[newIndex]);
        bool ok = false;
        switch (join.comparison) {
        case ComparisonOperator::Equal:
          ok = cmp == 0;
          break;
        case ComparisonOperator::NotEqual:
          ok = cmp != 0;
          break;
        case ComparisonOperator::GreaterThan:
          ok = cmp > 0;
          break;
        case ComparisonOperator::LessThan:
          ok = cmp < 0;
          break;
        case ComparisonOperator::GreaterThanOrEqual:
          ok = cmp >= 0;
          break;
        case ComparisonOperator::LessThanOrEqual:
          ok = cmp <= 0;
          break;
        default:
          throw std::invalid_argument(
              "INNER JOIN supports basic comparisons only");
        }
        if (ok) {
          auto row = oldRow;
          row.insert(row.end(), newRow.begin(), newRow.end());
          expanded.push_back(std::move(row));
        }
      }
    rows = std::move(expanded);
    qualifiers.push_back(qualifier);
    for (const auto &c : schemas[j + 1]) {
      combinedSchema.push_back({qualifier + "." + c.name, c.type});
      combinedTypes.push_back(c.type);
    }
  }
  std::optional<domain::CompiledWhereExpression> where;
  if (const auto *w = std::get_if<WhereExpression>(&select.where)) {
    auto q = *w;
    std::function<void(WhereExpression &)> qualify = [&](WhereExpression &e) {
      if (e.kind != WhereExpression::Kind::Predicate) {
        for (auto &c : e.children)
          qualify(c);
        return;
      }
      const auto name = e.predicate.columnName;
      const auto dot = name.find('.');
      if (dot != std::string::npos) {
        const auto alias = name.substr(0, dot), col = name.substr(dot + 1);
        if (std::find(qualifiers.begin(), qualifiers.end(), alias) ==
            qualifiers.end())
          throw std::invalid_argument("Unknown WHERE table reference: " +
                                      alias);
        e.predicate.columnName = alias + "." + col;
        return;
      }
      std::optional<std::string> found;
      for (std::size_t t = 0; t < schemas.size(); ++t)
        for (const auto &c : schemas[t])
          if (c.name == name) {
            if (found)
              throw std::invalid_argument("Ambiguous WHERE column: " + name);
            found = qualifiers[t] + "." + name;
          }
      if (!found)
        throw std::invalid_argument("Unknown WHERE column: " + name);
      e.predicate.columnName = *found;
    };
    qualify(q);
    where = domain::compileWhereExpression(q, combinedSchema, combinedTypes);
  }
  std::vector<std::size_t> projection;
  std::vector<ColumnDefinition> output;
  if (select.selectAll) {
    for (std::size_t i = 0; i < combinedSchema.size(); ++i) {
      projection.push_back(i);
      const auto &name = combinedSchema[i].name;
      const auto dot = name.find('.');
      const auto col = name.substr(dot + 1);
      std::size_t count = 0;
      for (const auto &sc : schemas)
        for (const auto &c : sc)
          if (c.name == col)
            ++count;
      output.push_back({count > 1 ? name : col, combinedSchema[i].type});
    }
  } else {
    if (select.columnNames.empty())
      throw std::invalid_argument("SELECT requires at least one column");
    for (std::size_t i = 0; i < select.columnNames.size(); ++i) {
      const auto name = select.columnNames[i];
      const auto dot = name.find('.');
      std::optional<std::size_t> idx;
      if (dot != std::string::npos) {
        const auto q = name.substr(0, dot), col = name.substr(dot + 1);
        auto qi = std::find(qualifiers.begin(), qualifiers.end(), q);
        if (qi == qualifiers.end())
          throw std::invalid_argument("Unknown SELECT table: " + q);
        auto ti =
            static_cast<std::size_t>(std::distance(qualifiers.begin(), qi));
        std::size_t off = 0;
        for (std::size_t k = 0; k < ti; ++k)
          off += schemas[k].size();
        for (std::size_t k = 0; k < schemas[ti].size(); ++k)
          if (schemas[ti][k].name == col)
            idx = off + k;
      } else {
        for (std::size_t k = 0; k < combinedSchema.size(); ++k)
          if (combinedSchema[k].name.substr(combinedSchema[k].name.find('.') +
                                            1) == name) {
            if (idx)
              throw std::invalid_argument("Ambiguous SELECT column: " + name);
            idx = k;
          }
      }
      if (!idx)
        throw std::invalid_argument("Unknown SELECT column: " + name);
      projection.push_back(*idx);
      auto outName = combinedSchema[*idx].name;
      const auto d = outName.find('.');
      const auto col = outName.substr(d + 1);
      std::size_t count = 0;
      for (const auto &sc : schemas)
        for (const auto &c : sc)
          if (c.name == col)
            ++count;
      if (i < select.columnAliases.size() && select.columnAliases[i])
        outName = *select.columnAliases[i];
      else if (count == 1)
        outName = col;
      output.push_back({outName, combinedSchema[*idx].type});
    }
  }
  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = std::move(output);
  for (auto &row : rows) {
    if (where && !domain::matchesWhereExpression(row, *where))
      continue;
    std::vector<SqlValue> projected;
    for (auto index : projection)
      projected.push_back(row[index]);
    result.rows.push_back(std::move(projected));
  }
  return result;
}

QueryExecutionResult commandResult(std::string message,
                                   uint32_t affectedRows = 0) {
  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Command;
  result.message = std::move(message);
  result.affectedRows = affectedRows;
  return result;
}

} // namespace

ExecuteQueryAction::ExecuteQueryAction(RdbServicePort &rdbServicePort,
                                       const unidb::sql::SqlParser &sqlParser)
    : rdbServicePort(rdbServicePort), sqlParser(sqlParser) {}

QueryExecutionOutcome ExecuteQueryAction::execute(std::string_view sql) const {
  const unidb::sql::SqlParseResult parseResult = sqlParser.parse(sql);
  if (const auto *error =
          std::get_if<unidb::sql::SqlParseError>(&parseResult)) {
    const auto code = error->code == unidb::sql::SqlParseErrorCode::SyntaxError
                          ? QueryExecutionErrorCode::SyntaxError
                          : QueryExecutionErrorCode::UnsupportedStatement;
    return QueryExecutionError{code, error->message};
  }

  try {
    return executeStatement(std::get<SqlStatement>(parseResult));
  } catch (const std::exception &error) {
    return executionError(error.what());
  } catch (...) {
    return executionError("Unknown query execution failure");
  }
}

QueryExecutionOutcome
ExecuteQueryAction::executeStatement(const SqlStatement &statement) const {
  if (const auto *create = std::get_if<CreateTableStatement>(&statement)) {
    if (create->columns.empty()) {
      throw std::invalid_argument("CREATE TABLE requires at least one column");
    }
    std::unordered_set<std::string> names;
    for (const auto &column : create->columns) {
      if (column.name.empty() || !names.insert(column.name).second) {
        throw std::invalid_argument(
            "Column names must be non-empty and unique");
      }
      if (column.type != SqlType::Int && column.type != SqlType::Text) {
        throw std::invalid_argument("Column type is outside the V1 SQL subset");
      }
    }
    rdbServicePort.createTable(*create);
    return commandResult("Table created");
  }

  if (const auto *insert = std::get_if<InsertStatement>(&statement)) {
    const auto schema = rdbServicePort.getTableDefinition(insert->tableName);
    const auto types = schemaTypes(schema);
    if (insert->rows.empty()) {
      throw std::invalid_argument("INSERT requires at least one VALUES row");
    }

    std::vector<std::vector<char>> encodedRows;
    encodedRows.reserve(insert->rows.size());
    for (const auto &values : insert->rows) {
      if (insert->columnNames.size() != values.size()) {
        throw std::invalid_argument(
            "INSERT column and value counts must match");
      }

      std::vector<SqlValue> orderedValues(schema.size());
      std::vector<bool> assigned(schema.size(), false);
      for (std::size_t inputIndex = 0; inputIndex < insert->columnNames.size();
           ++inputIndex) {
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == insert->columnNames[inputIndex];
            });
        if (found == schema.end()) {
          throw std::invalid_argument("Unknown INSERT column: " +
                                      insert->columnNames[inputIndex]);
        }
        const auto schemaIndex =
            static_cast<std::size_t>(std::distance(schema.begin(), found));
        if (assigned[schemaIndex]) {
          throw std::invalid_argument("Duplicate INSERT column: " +
                                      insert->columnNames[inputIndex]);
        }
        const bool valueIsInt =
            std::holds_alternative<std::int64_t>(values[inputIndex]);
        if ((types[schemaIndex] == SqlType::Int) != valueIsInt) {
          throw std::invalid_argument(
              "INSERT value type does not match column: " + found->name);
        }
        orderedValues[schemaIndex] = values[inputIndex];
        assigned[schemaIndex] = true;
      }

      if (std::find(assigned.begin(), assigned.end(), false) !=
          assigned.end()) {
        throw std::invalid_argument(
            "V1 INSERT requires a value for every table column");
      }
      encodedRows.push_back(SqlTupleCodec::encode(types, orderedValues));
    }

    std::uint32_t insertedRows = 0;
    try {
      for (const auto &bytes : encodedRows) {
        try {
          rdbServicePort.insertRow(insert->tableName, bytes);
          ++insertedRows;
        } catch (const RowInsertedButPostInsertUpdateFailed &) {
          ++insertedRows;
          throw;
        }
      }
    } catch (const std::exception &error) {
      return executionError("INSERT failed after " +
                            std::to_string(insertedRows) +
                            " row(s) were inserted: " + error.what());
    }
    return commandResult("Rows inserted", insertedRows);
  }

  if (const auto *unionAll = std::get_if<UnionAllStatement>(&statement)) {
    if (unionAll->selects.size() < 2) {
      throw std::invalid_argument(
          "UNION ALL requires at least two SELECT branches");
    }

    QueryExecutionResult combined;
    bool firstBranch = true;
    for (const auto &select : unionAll->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      const auto &branchResult = std::get<QueryExecutionResult>(branchOutcome);
      if (branchResult.kind != QueryExecutionResultKind::Rows) {
        throw std::invalid_argument("UNION ALL branches must return rows");
      }

      if (firstBranch) {
        combined.kind = QueryExecutionResultKind::Rows;
        combined.message = "Rows selected";
        combined.columns = branchResult.columns;
        firstBranch = false;
      } else {
        if (branchResult.columns.size() != combined.columns.size()) {
          throw std::invalid_argument(
              "UNION ALL branches must return the same number of columns");
        }
        for (std::size_t index = 0; index < combined.columns.size(); ++index) {
          if (branchResult.columns[index].type !=
              combined.columns[index].type) {
            throw std::invalid_argument(
                "UNION ALL columns must have matching types at each position");
          }
        }
      }

      combined.rows.insert(combined.rows.end(), branchResult.rows.begin(),
                           branchResult.rows.end());
    }
    return combined;
  }

  if (const auto *unionDistinct = std::get_if<UnionStatement>(&statement)) {
    if (unionDistinct->selects.size() < 2) {
      throw std::invalid_argument(
          "UNION requires at least two SELECT branches");
    }

    QueryExecutionResult combined;
    std::set<std::vector<SqlValue>> seenRows;
    bool firstBranch = true;
    for (const auto &select : unionDistinct->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      const auto &branchResult = std::get<QueryExecutionResult>(branchOutcome);
      if (branchResult.kind != QueryExecutionResultKind::Rows) {
        throw std::invalid_argument("UNION branches must return rows");
      }

      if (firstBranch) {
        combined.kind = QueryExecutionResultKind::Rows;
        combined.message = "Rows selected";
        combined.columns = branchResult.columns;
        firstBranch = false;
      } else {
        if (branchResult.columns.size() != combined.columns.size()) {
          throw std::invalid_argument(
              "UNION branches must return the same number of columns");
        }
        for (std::size_t index = 0; index < combined.columns.size(); ++index) {
          if (branchResult.columns[index].type !=
              combined.columns[index].type) {
            throw std::invalid_argument(
                "UNION columns must have matching types at each position");
          }
        }
      }

      for (const auto &row : branchResult.rows) {
        if (seenRows.insert(row).second) {
          combined.rows.push_back(row);
        }
      }
    }
    return combined;
  }

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    if (select->crossJoin.has_value()) {
      if (!select->innerJoins.empty()) {
        throw std::invalid_argument(
            "Mixing CROSS JOIN and INNER JOIN is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->crossJoin->tableName);
      schemaTypes(leftSchema);
      schemaTypes(rightSchema);
      return executeCrossJoin(rdbServicePort, *select, leftSchema, rightSchema);
    }
    if (!select->innerJoins.empty()) {
      std::vector<std::vector<ColumnDefinition>> schemas;
      schemas.push_back(rdbServicePort.getTableDefinition(select->tableName));
      for (const auto &join : select->innerJoins)
        schemas.push_back(rdbServicePort.getTableDefinition(join.tableName));
      for (const auto &schema : schemas)
        schemaTypes(schema);
      if (select->innerJoins.size() == 1) {
        validateInnerJoinColumns(*select, schemas[0], schemas[1]);
        return executeInnerJoin(rdbServicePort, *select, schemas[0],
                                schemas[1]);
      }
      return executeMultipleInnerJoins(rdbServicePort, *select, schemas);
    }
    const auto schema = rdbServicePort.getTableDefinition(select->tableName);
    const auto types = schemaTypes(schema);

    const auto *where = std::get_if<WhereExpression>(&select->where);
    std::optional<domain::CompiledWhereExpression> compiledWhere;
    if (where != nullptr) {
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      const auto normalized =
          normalizeSingleTableWhereColumns(*where, tableQualifier);
      compiledWhere = domain::compileWhereExpression(normalized, schema, types);
    }

    std::vector<std::size_t> selectedIndexes;
    std::vector<ColumnDefinition> selectedColumns;
    std::vector<std::optional<std::vector<domain::CompiledWhereExpression>>>
        compiledCases;
    std::vector<std::optional<domain::CompiledIntegerExpression>>
        compiledIntegers;
    std::vector<std::optional<domain::CompiledTextFunction>>
        compiledTextFunctions;
    if (select->aggregate.has_value() && select->groupBy.empty()) {
      // Ungrouped aggregate queries produce a single computed output column.
    } else if (select->selectAll) {
      selectedIndexes.reserve(schema.size());
      selectedColumns = schema;
      for (std::size_t index = 0; index < schema.size(); ++index) {
        selectedIndexes.push_back(index);
      }
    } else {
      if (select->columnNames.empty()) {
        throw std::invalid_argument("SELECT requires at least one column");
      }
      selectedIndexes.reserve(select->columnNames.size());
      selectedColumns.reserve(select->columnNames.size());
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      if (!select->caseExpressions.empty() &&
          select->caseExpressions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT CASE expression count does not match columns");
      }
      if (!select->integerExpressions.empty() &&
          select->integerExpressions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT integer expression count does not match columns");
      }
      if (!select->textFunctions.empty() &&
          select->textFunctions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT text function count does not match columns");
      }
      compiledCases.reserve(select->columnNames.size());
      compiledIntegers.reserve(select->columnNames.size());
      compiledTextFunctions.reserve(select->columnNames.size());
      for (std::size_t itemIndex = 0; itemIndex < select->columnNames.size();
           ++itemIndex) {
        if (itemIndex < select->textFunctions.size() &&
            select->textFunctions[itemIndex].has_value()) {
          if (!select->innerJoins.empty()) {
            throw std::invalid_argument(
                "Scalar text functions are not yet supported with JOIN");
          }
          const auto &expression = *select->textFunctions[itemIndex];
          const auto compiled =
              compileTextFunction(expression, schema, types, tableQualifier);
          selectedIndexes.push_back(schema.size());
          const auto alias = itemIndex < select->columnAliases.size()
                                 ? select->columnAliases[itemIndex]
                                 : std::optional<std::string>{};
          const auto functionName = textFunctionName(expression.function);
          const auto outputType = expression.function == TextFunction::Length
                                      ? SqlType::Int
                                      : SqlType::Text;
          selectedColumns.push_back(
              {alias.value_or(functionName + "(" + expression.columnName + ")"),
               outputType});
          compiledCases.push_back(std::nullopt);
          compiledIntegers.push_back(std::nullopt);
          compiledTextFunctions.push_back(compiled);
          continue;
        }
        if (itemIndex < select->integerExpressions.size() &&
            select->integerExpressions[itemIndex].has_value()) {
          if (!select->innerJoins.empty()) {
            throw std::invalid_argument(
                "Integer expressions are not yet supported with JOIN");
          }
          const auto compiled =
              compileIntegerExpression(*select->integerExpressions[itemIndex],
                                       schema, types, tableQualifier);
          selectedIndexes.push_back(schema.size());
          const auto alias = itemIndex < select->columnAliases.size()
                                 ? select->columnAliases[itemIndex]
                                 : std::optional<std::string>{};
          selectedColumns.push_back(
              {alias.value_or("EXPR" + std::to_string(itemIndex + 1)),
               SqlType::Int});
          compiledCases.push_back(std::nullopt);
          compiledIntegers.push_back(compiled);
          compiledTextFunctions.push_back(std::nullopt);
          continue;
        }
        if (itemIndex < select->caseExpressions.size() &&
            select->caseExpressions[itemIndex].has_value()) {
          if (!select->innerJoins.empty()) {
            throw std::invalid_argument(
                "CASE expressions are not yet supported with JOIN");
          }
          const auto &caseExpression = *select->caseExpressions[itemIndex];
          const SqlValue &elseValue = caseExpression.elseResult;
          const bool resultIsInt =
              std::holds_alternative<std::int64_t>(elseValue);
          std::vector<domain::CompiledWhereExpression> branches;
          branches.reserve(caseExpression.branches.size());
          for (const auto &branch : caseExpression.branches) {
            if (std::holds_alternative<std::int64_t>(branch.result) !=
                resultIsInt) {
              throw std::invalid_argument(
                  "CASE result values must have the same type");
            }
            const auto normalized = normalizeSingleTableWhereColumns(
                branch.condition, tableQualifier);
            branches.push_back(
                domain::compileWhereExpression(normalized, schema, types));
          }
          selectedIndexes.push_back(schema.size());
          selectedColumns.push_back(
              {select->columnAliases[itemIndex].value_or("CASE"),
               resultIsInt ? SqlType::Int : SqlType::Text});
          compiledCases.push_back(std::move(branches));
          compiledIntegers.push_back(std::nullopt);
          compiledTextFunctions.push_back(std::nullopt);
          continue;
        }
        const auto &columnName = select->columnNames[itemIndex];
        const auto normalizedColumnName =
            normalizeSingleTableColumnName(columnName, tableQualifier);
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == normalizedColumnName;
            });
        if (found == schema.end()) {
          throw std::invalid_argument("Unknown SELECT column: " + columnName);
        }
        const auto schemaIndex =
            static_cast<std::size_t>(std::distance(schema.begin(), found));
        selectedIndexes.push_back(schemaIndex);
        selectedColumns.push_back(*found);
        compiledCases.push_back(std::nullopt);
        compiledIntegers.push_back(std::nullopt);
        compiledTextFunctions.push_back(std::nullopt);
      }

      if (!select->columnAliases.empty() &&
          select->columnAliases.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT alias count does not match columns");
      }
      std::unordered_set<std::string> outputNames;
      for (std::size_t index = 0; index < selectedColumns.size(); ++index) {
        if (index < select->columnAliases.size() &&
            select->columnAliases[index].has_value()) {
          selectedColumns[index].name = *select->columnAliases[index];
        } else if (index < select->caseExpressions.size() &&
                   select->caseExpressions[index].has_value()) {
          selectedColumns[index].name = "CASE";
        }
        if (!outputNames.insert(selectedColumns[index].name).second) {
          throw std::invalid_argument("Duplicate SELECT output column name: " +
                                      selectedColumns[index].name);
        }
      }
    }

    const bool hasCaseExpression = std::any_of(
        select->caseExpressions.begin(), select->caseExpressions.end(),
        [](const auto &expression) { return expression.has_value(); });
    const bool hasTextFunction = std::any_of(
        select->textFunctions.begin(), select->textFunctions.end(),
        [](const auto &expression) { return expression.has_value(); });
    if (hasTextFunction && select->distinct) {
      throw std::invalid_argument(
          "SELECT DISTINCT cannot be combined with scalar text functions yet");
    }
    if (hasCaseExpression && select->distinct) {
      throw std::invalid_argument(
          "SELECT DISTINCT cannot be combined with CASE expressions yet");
    }

    struct OrderIndex {
      std::size_t index;
      bool ascending;
      bool projected;
    };
    std::vector<OrderIndex> orderIndexes;
    orderIndexes.reserve(select->orderBy.size());
    const std::string tableQualifier =
        select->tableAlias.value_or(select->tableName);
    if (select->groupBy.empty()) {
      for (const auto &order : select->orderBy) {
        const auto alias =
            std::find_if(selectedColumns.begin(), selectedColumns.end(),
                         [&](const auto &column) {
                           return column.name == order.columnName;
                         });
        if (alias != selectedColumns.end()) {
          const auto projectionIndex = static_cast<std::size_t>(
              std::distance(selectedColumns.begin(), alias));
          orderIndexes.push_back({projectionIndex, order.ascending, true});
          continue;
        }

        const auto normalizedOrderColumn =
            normalizeSingleTableColumnName(order.columnName, tableQualifier);
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == normalizedOrderColumn;
            });
        if (found == schema.end()) {
          throw std::invalid_argument("Unknown ORDER BY column: " +
                                      order.columnName);
        }
        const auto orderIndex =
            static_cast<std::size_t>(std::distance(schema.begin(), found));
        if (select->distinct &&
            std::find(selectedIndexes.begin(), selectedIndexes.end(),
                      orderIndex) == selectedIndexes.end()) {
          throw std::invalid_argument("ORDER BY column must appear in SELECT "
                                      "list when DISTINCT is used: " +
                                      order.columnName);
        }
        orderIndexes.push_back({orderIndex, order.ascending, false});
      }
    }

    std::optional<std::size_t> aggregateIndex;
    std::vector<std::size_t> groupIndexes;
    if (select->having.has_value() && select->groupBy.empty()) {
      throw std::invalid_argument("HAVING requires GROUP BY");
    }
    if (!select->groupBy.empty()) {
      if (!select->aggregate.has_value()) {
        throw std::invalid_argument(
            "GROUP BY requires one aggregate expression in SELECT");
      }
      if (select->distinct) {
        throw std::invalid_argument(
            "Grouped aggregate SELECT cannot be combined with DISTINCT");
      }
      if (select->columnNames != select->groupBy) {
        throw std::invalid_argument(
            "SELECT columns must exactly match GROUP BY columns");
      }
      for (const auto &groupColumn : select->groupBy) {
        const auto normalizedGroupColumn =
            normalizeSingleTableColumnName(groupColumn, tableQualifier);
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == normalizedGroupColumn;
            });
        if (found == schema.end()) {
          throw std::invalid_argument("Unknown GROUP BY column: " +
                                      groupColumn);
        }
        groupIndexes.push_back(
            static_cast<std::size_t>(std::distance(schema.begin(), found)));
      }
    }
    if (select->aggregate.has_value()) {
      const auto &aggregate = *select->aggregate;
      if (select->distinct ||
          (select->groupBy.empty() &&
           (!select->orderBy.empty() || select->limit.has_value() ||
            select->offset != 0))) {
        throw std::invalid_argument(
            "Aggregate SELECT cannot be combined with DISTINCT, ORDER BY, "
            "LIMIT, or OFFSET");
      }
      if (aggregate.columnName.has_value()) {
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == *aggregate.columnName;
            });
        if (found == schema.end()) {
          throw std::invalid_argument("Unknown aggregate column: " +
                                      *aggregate.columnName);
        }
        aggregateIndex =
            static_cast<std::size_t>(std::distance(schema.begin(), found));
      } else if (aggregate.function != AggregateFunction::Count) {
        throw std::invalid_argument("Only COUNT supports '*'");
      }
      if ((aggregate.function == AggregateFunction::Sum ||
           aggregate.function == AggregateFunction::Avg) &&
          (!aggregateIndex.has_value() ||
           types[*aggregateIndex] != SqlType::Int)) {
        throw std::invalid_argument("SUM and AVG require an INT column");
      }
      if (!select->groupBy.empty() && select->having.has_value()) {
        const auto &having = *select->having;
        if (having.aggregate.function != aggregate.function ||
            having.aggregate.columnName != aggregate.columnName) {
          throw std::invalid_argument(
              "HAVING aggregate must match the SELECT aggregate");
        }
        const SqlType resultType =
            aggregate.function == AggregateFunction::Min ||
                    aggregate.function == AggregateFunction::Max
                ? types[*aggregateIndex]
                : SqlType::Int;
        const bool valueIsInt =
            std::holds_alternative<std::int64_t>(having.value);
        if ((resultType == SqlType::Int) != valueIsInt) {
          throw std::invalid_argument(
              "HAVING value type does not match aggregate result type");
        }
      }
    }

    struct GroupOrderIndex {
      std::size_t index;
      bool ascending;
    };
    std::vector<GroupOrderIndex> groupedOrderIndexes;
    if (!select->groupBy.empty()) {
      const auto &aggregate = *select->aggregate;
      const std::string functionName =
          aggregate.function == AggregateFunction::Count ? "COUNT"
          : aggregate.function == AggregateFunction::Min ? "MIN"
          : aggregate.function == AggregateFunction::Max ? "MAX"
          : aggregate.function == AggregateFunction::Sum ? "SUM"
                                                         : "AVG";
      const std::string aggregateName = aggregate.alias.value_or(
          functionName + "(" +
          (aggregate.columnName.has_value() ? *aggregate.columnName : "*") +
          ")");
      groupedOrderIndexes.reserve(select->orderBy.size());
      for (const auto &order : select->orderBy) {
        const auto projected =
            std::find_if(selectedColumns.begin(), selectedColumns.end(),
                         [&](const auto &column) {
                           return column.name == order.columnName;
                         });
        if (projected != selectedColumns.end()) {
          groupedOrderIndexes.push_back(
              {static_cast<std::size_t>(
                   std::distance(selectedColumns.begin(), projected)),
               order.ascending});
          continue;
        }
        if (order.columnName == aggregateName) {
          groupedOrderIndexes.push_back(
              {selectedColumns.size(), order.ascending});
          continue;
        }

        const auto normalizedOrderColumn =
            normalizeSingleTableColumnName(order.columnName, tableQualifier);
        const auto groupIndex = std::find_if(
            groupIndexes.begin(), groupIndexes.end(), [&](const auto index) {
              return schema[index].name == normalizedOrderColumn;
            });
        if (groupIndex == groupIndexes.end()) {
          throw std::invalid_argument("Unknown GROUP BY ORDER BY column: " +
                                      order.columnName);
        }
        groupedOrderIndexes.push_back({static_cast<std::size_t>(std::distance(
                                           groupIndexes.begin(), groupIndex)),
                                       order.ascending});
      }
    }

    const auto encodedRows = rdbServicePort.selectAllRows(select->tableName);
    std::vector<std::vector<SqlValue>> matchedRows;
    matchedRows.reserve(encodedRows.size());
    for (const auto &bytes : encodedRows) {
      auto decoded = SqlTupleCodec::decode(types, bytes);
      if (compiledWhere.has_value() &&
          !domain::matchesWhereExpression(decoded, *compiledWhere)) {
        continue;
      }
      matchedRows.push_back(std::move(decoded));
    }

    if (!select->groupBy.empty()) {
      const auto &aggregate = *select->aggregate;
      const std::string functionName =
          aggregate.function == AggregateFunction::Count ? "COUNT"
          : aggregate.function == AggregateFunction::Min ? "MIN"
          : aggregate.function == AggregateFunction::Max ? "MAX"
          : aggregate.function == AggregateFunction::Sum ? "SUM"
                                                         : "AVG";
      const SqlType aggregateType =
          aggregate.function == AggregateFunction::Min ||
                  aggregate.function == AggregateFunction::Max
              ? types[*aggregateIndex]
              : SqlType::Int;
      using GroupRows = std::map<std::vector<SqlValue>,
                                 std::vector<const std::vector<SqlValue> *>>;
      GroupRows groups;
      for (const auto &row : matchedRows) {
        std::vector<SqlValue> key;
        key.reserve(groupIndexes.size());
        for (const auto index : groupIndexes) {
          key.push_back(row[index]);
        }
        groups[std::move(key)].push_back(&row);
      }

      QueryExecutionResult result;
      result.kind = QueryExecutionResultKind::Rows;
      result.message = "Rows selected";
      result.columns = selectedColumns;
      const std::string aggregateName = aggregate.alias.value_or(
          functionName + "(" +
          (aggregate.columnName.has_value() ? *aggregate.columnName : "*") +
          ")");
      if (std::any_of(result.columns.begin(), result.columns.end(),
                      [&](const auto &column) {
                        return column.name == aggregateName;
                      })) {
        throw std::invalid_argument("Duplicate SELECT output column name: " +
                                    aggregateName);
      }
      result.columns.push_back({aggregateName, aggregateType});

      for (const auto &[key, rows] : groups) {
        SqlValue aggregateValue = std::int64_t{0};
        if (aggregate.function == AggregateFunction::Count) {
          aggregateValue = static_cast<std::int64_t>(rows.size());
        } else {
          if (rows.empty()) {
            continue;
          }
          if (aggregate.function == AggregateFunction::Min ||
              aggregate.function == AggregateFunction::Max) {
            aggregateValue = (*rows.front())[*aggregateIndex];
            for (const auto *row : rows) {
              const int comparison = domain::compareSqlValues(
                  (*row)[*aggregateIndex], aggregateValue);
              if ((aggregate.function == AggregateFunction::Min &&
                   comparison < 0) ||
                  (aggregate.function == AggregateFunction::Max &&
                   comparison > 0)) {
                aggregateValue = (*row)[*aggregateIndex];
              }
            }
          } else {
            std::int64_t sum = 0;
            for (const auto *row : rows) {
              const auto value =
                  std::get<std::int64_t>((*row)[*aggregateIndex]);
              if ((value > 0 &&
                   sum > std::numeric_limits<std::int64_t>::max() - value) ||
                  (value < 0 &&
                   sum < std::numeric_limits<std::int64_t>::min() - value)) {
                throw std::invalid_argument("SUM result exceeds INT64 range");
              }
              sum += value;
            }
            if (aggregate.function == AggregateFunction::Avg) {
              sum /= static_cast<std::int64_t>(rows.size());
            }
            aggregateValue = sum;
          }
        }

        if (select->having.has_value()) {
          const int comparison =
              domain::compareSqlValues(aggregateValue, select->having->value);
          bool matches = false;
          switch (select->having->comparison) {
          case ComparisonOperator::Equal:
            matches = comparison == 0;
            break;
          case ComparisonOperator::NotEqual:
            matches = comparison != 0;
            break;
          case ComparisonOperator::GreaterThan:
            matches = comparison > 0;
            break;
          case ComparisonOperator::LessThan:
            matches = comparison < 0;
            break;
          case ComparisonOperator::GreaterThanOrEqual:
            matches = comparison >= 0;
            break;
          case ComparisonOperator::LessThanOrEqual:
            matches = comparison <= 0;
            break;
          default:
            throw std::invalid_argument(
                "HAVING supports only basic comparison operators");
          }
          if (!matches) {
            continue;
          }
        }

        auto outputRow = key;
        outputRow.push_back(std::move(aggregateValue));
        result.rows.push_back(std::move(outputRow));
      }

      if (!groupedOrderIndexes.empty()) {
        std::stable_sort(result.rows.begin(), result.rows.end(),
                         [&](const auto &left, const auto &right) {
                           for (const auto &order : groupedOrderIndexes) {
                             const int comparison = domain::compareSqlValues(
                                 left[order.index], right[order.index]);
                             if (comparison != 0) {
                               return order.ascending ? comparison < 0
                                                      : comparison > 0;
                             }
                           }
                           return false;
                         });
      }

      const std::size_t offset = select->offset > result.rows.size()
                                     ? result.rows.size()
                                     : static_cast<std::size_t>(select->offset);
      if (offset > 0) {
        result.rows.erase(result.rows.begin(), result.rows.begin() + offset);
      }
      if (select->limit.has_value() && *select->limit < result.rows.size()) {
        result.rows.resize(static_cast<std::size_t>(*select->limit));
      }
      return result;
    }

    if (select->aggregate.has_value()) {
      const auto &aggregate = *select->aggregate;
      const std::string functionName =
          aggregate.function == AggregateFunction::Count ? "COUNT"
          : aggregate.function == AggregateFunction::Min ? "MIN"
          : aggregate.function == AggregateFunction::Max ? "MAX"
          : aggregate.function == AggregateFunction::Sum ? "SUM"
                                                         : "AVG";
      const std::string outputName = aggregate.alias.value_or(
          functionName + "(" +
          (aggregate.columnName.has_value() ? *aggregate.columnName : "*") +
          ")");
      QueryExecutionResult result;
      result.kind = QueryExecutionResultKind::Rows;
      result.message = "Rows selected";
      const SqlType outputType =
          aggregate.function == AggregateFunction::Min ||
                  aggregate.function == AggregateFunction::Max
              ? types[*aggregateIndex]
              : SqlType::Int;
      result.columns.push_back({outputName, outputType});

      if (aggregate.function == AggregateFunction::Count) {
        result.rows.push_back({static_cast<std::int64_t>(matchedRows.size())});
        return result;
      }
      if (matchedRows.empty()) {
        throw std::invalid_argument(
            functionName + " cannot be evaluated on an empty result set");
      }

      if (aggregate.function == AggregateFunction::Min ||
          aggregate.function == AggregateFunction::Max) {
        SqlValue value = matchedRows.front()[*aggregateIndex];
        for (const auto &row : matchedRows) {
          const int comparison =
              domain::compareSqlValues(row[*aggregateIndex], value);
          if ((aggregate.function == AggregateFunction::Min &&
               comparison < 0) ||
              (aggregate.function == AggregateFunction::Max &&
               comparison > 0)) {
            value = row[*aggregateIndex];
          }
        }
        result.rows.push_back({std::move(value)});
        return result;
      }

      std::int64_t sum = 0;
      for (const auto &row : matchedRows) {
        const auto value = std::get<std::int64_t>(row[*aggregateIndex]);
        if ((value > 0 &&
             sum > std::numeric_limits<std::int64_t>::max() - value) ||
            (value < 0 &&
             sum < std::numeric_limits<std::int64_t>::min() - value)) {
          throw std::invalid_argument("SUM result exceeds INT64 range");
        }
        sum += value;
      }
      if (aggregate.function == AggregateFunction::Avg) {
        sum /= static_cast<std::int64_t>(matchedRows.size());
      }
      result.rows.push_back({sum});
      return result;
    }

    const bool hasIntegerExpression = std::any_of(
        select->integerExpressions.begin(), select->integerExpressions.end(),
        [](const auto &expression) { return expression.has_value(); });
    if (hasIntegerExpression && select->distinct) {
      throw std::invalid_argument(
          "SELECT DISTINCT cannot be combined with integer expressions yet");
    }

    if (select->distinct) {
      std::set<std::vector<SqlValue>> seenProjections;
      matchedRows.erase(
          std::remove_if(
              matchedRows.begin(), matchedRows.end(),
              [&](const auto &row) {
                std::vector<SqlValue> projection;
                projection.reserve(selectedIndexes.size());
                for (const auto index : selectedIndexes) {
                  projection.push_back(row[index]);
                }
                return !seenProjections.insert(std::move(projection)).second;
              }),
          matchedRows.end());
    }

    const auto projectRow = [&](const std::vector<SqlValue> &decoded) {
      std::vector<SqlValue> projected;
      projected.reserve(selectedIndexes.size());
      for (std::size_t index = 0; index < selectedIndexes.size(); ++index) {
        if (index < compiledTextFunctions.size() &&
            compiledTextFunctions[index].has_value()) {
          projected.push_back(domain::evaluateTextFunction(
              decoded, *compiledTextFunctions[index]));
        } else if (index < compiledIntegers.size() &&
                   compiledIntegers[index].has_value()) {
          projected.push_back(domain::evaluateIntegerExpression(
              decoded, *compiledIntegers[index]));
        } else if (index < compiledCases.size() &&
                   compiledCases[index].has_value()) {
          const auto &caseExpression = *select->caseExpressions[index];
          bool matched = false;
          for (std::size_t branchIndex = 0;
               branchIndex < compiledCases[index]->size(); ++branchIndex) {
            if (domain::matchesWhereExpression(
                    decoded, (*compiledCases[index])[branchIndex])) {
              projected.push_back(caseExpression.branches[branchIndex].result);
              matched = true;
              break;
            }
          }
          if (!matched) {
            projected.push_back(caseExpression.elseResult);
          }
        } else {
          projected.push_back(decoded[selectedIndexes[index]]);
        }
      }
      return projected;
    };

    const bool ordersByProjection =
        std::any_of(orderIndexes.begin(), orderIndexes.end(),
                    [](const auto &order) { return order.projected; });
    std::vector<std::vector<SqlValue>> projectedRows;
    if (ordersByProjection) {
      projectedRows.reserve(matchedRows.size());
      for (const auto &row : matchedRows) {
        projectedRows.push_back(projectRow(row));
      }
    }

    if (!orderIndexes.empty()) {
      std::vector<std::size_t> rowOrder(matchedRows.size());
      for (std::size_t index = 0; index < rowOrder.size(); ++index) {
        rowOrder[index] = index;
      }
      std::stable_sort(
          rowOrder.begin(), rowOrder.end(),
          [&](std::size_t leftIndex, std::size_t rightIndex) {
            for (const auto &order : orderIndexes) {
              const auto &left = order.projected
                                     ? projectedRows[leftIndex][order.index]
                                     : matchedRows[leftIndex][order.index];
              const auto &right = order.projected
                                      ? projectedRows[rightIndex][order.index]
                                      : matchedRows[rightIndex][order.index];
              const int comparison = domain::compareSqlValues(left, right);
              if (comparison != 0) {
                return order.ascending ? comparison < 0 : comparison > 0;
              }
            }
            return false;
          });

      std::vector<std::vector<SqlValue>> sortedRows;
      std::vector<std::vector<SqlValue>> sortedProjections;
      sortedRows.reserve(matchedRows.size());
      if (ordersByProjection) {
        sortedProjections.reserve(projectedRows.size());
      }
      for (const auto index : rowOrder) {
        sortedRows.push_back(std::move(matchedRows[index]));
        if (ordersByProjection) {
          sortedProjections.push_back(std::move(projectedRows[index]));
        }
      }
      matchedRows = std::move(sortedRows);
      if (ordersByProjection) {
        projectedRows = std::move(sortedProjections);
      }
    }

    const std::size_t offset = select->offset > matchedRows.size()
                                   ? matchedRows.size()
                                   : static_cast<std::size_t>(select->offset);
    if (offset > 0) {
      matchedRows.erase(matchedRows.begin(), matchedRows.begin() + offset);
      if (ordersByProjection) {
        projectedRows.erase(projectedRows.begin(),
                            projectedRows.begin() + offset);
      }
    }
    if (select->limit.has_value() && *select->limit < matchedRows.size()) {
      const auto limit = static_cast<std::size_t>(*select->limit);
      matchedRows.resize(limit);
      if (ordersByProjection) {
        projectedRows.resize(limit);
      }
    }
    if (!ordersByProjection) {
      projectedRows.reserve(matchedRows.size());
      for (const auto &row : matchedRows) {
        projectedRows.push_back(projectRow(row));
      }
    }

    QueryExecutionResult result;
    result.kind = QueryExecutionResultKind::Rows;
    result.message = "Rows selected";
    result.columns = std::move(selectedColumns);
    result.rows = std::move(projectedRows);
    return result;
  }

  return QueryExecutionError{QueryExecutionErrorCode::UnsupportedStatement,
                             "Statement is outside the V1 SQL subset"};
}

} // namespace QueryExecutionService
