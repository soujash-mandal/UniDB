#include "ExecuteQueryAction.h"

#include "../adapter/SqlTupleCodec.h"

#include <algorithm>
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
using unidb::sql::SelectStatement;
using unidb::sql::SqlStatement;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;
using unidb::sql::UnionAllStatement;
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

int compareSqlValues(const SqlValue &left, const SqlValue &right) {
  if (left.index() != right.index()) {
    throw std::invalid_argument(
        "WHERE comparison value type does not match column type");
  }
  if (const auto *leftInt = std::get_if<std::int64_t>(&left)) {
    const auto rightInt = std::get<std::int64_t>(right);
    return (*leftInt > rightInt) - (*leftInt < rightInt);
  }
  const auto &leftText = std::get<std::string>(left);
  const auto &rightText = std::get<std::string>(right);
  return (leftText > rightText) - (leftText < rightText);
}

bool matchesLikePattern(const std::string &value, const std::string &pattern) {
  std::size_t valueIndex = 0;
  std::size_t patternIndex = 0;
  std::size_t lastPercent = std::string::npos;
  std::size_t retryValueIndex = 0;

  while (valueIndex < value.size()) {
    if (patternIndex < pattern.size() &&
        (pattern[patternIndex] == '_' ||
         pattern[patternIndex] == value[valueIndex])) {
      ++valueIndex;
      ++patternIndex;
    } else if (patternIndex < pattern.size() && pattern[patternIndex] == '%') {
      lastPercent = patternIndex++;
      retryValueIndex = valueIndex;
    } else if (lastPercent != std::string::npos) {
      patternIndex = lastPercent + 1;
      valueIndex = ++retryValueIndex;
    } else {
      return false;
    }
  }

  while (patternIndex < pattern.size() && pattern[patternIndex] == '%') {
    ++patternIndex;
  }
  return patternIndex == pattern.size();
}

bool matchesWhere(const SqlValue &rowValue, const WhereEquals &predicate) {
  if (predicate.comparison == ComparisonOperator::Like ||
      predicate.comparison == ComparisonOperator::NotLike) {
    const auto *textValue = std::get_if<std::string>(&rowValue);
    const auto *pattern = std::get_if<std::string>(&predicate.value);
    if (textValue == nullptr || pattern == nullptr) {
      throw std::invalid_argument("LIKE predicates require TEXT values");
    }
    const bool matched = matchesLikePattern(*textValue, *pattern);
    return predicate.comparison == ComparisonOperator::Like ? matched
                                                            : !matched;
  }
  if (predicate.comparison == ComparisonOperator::Between ||
      predicate.comparison == ComparisonOperator::NotBetween) {
    if (!predicate.upperBound.has_value()) {
      throw std::invalid_argument(
          "BETWEEN predicates require two range bounds");
    }
    const bool withinRange =
        compareSqlValues(rowValue, predicate.value) >= 0 &&
        compareSqlValues(rowValue, *predicate.upperBound) <= 0;
    return predicate.comparison == ComparisonOperator::Between ? withinRange
                                                               : !withinRange;
  }
  if (predicate.comparison == ComparisonOperator::In ||
      predicate.comparison == ComparisonOperator::NotIn) {
    const bool found =
        std::any_of(predicate.values.begin(), predicate.values.end(),
                    [&](const auto &value) {
                      return compareSqlValues(rowValue, value) == 0;
                    });
    return predicate.comparison == ComparisonOperator::In ? found : !found;
  }

  const int comparison = compareSqlValues(rowValue, predicate.value);
  switch (predicate.comparison) {
  case ComparisonOperator::Equal:
    return comparison == 0;
  case ComparisonOperator::NotEqual:
    return comparison != 0;
  case ComparisonOperator::GreaterThan:
    return comparison > 0;
  case ComparisonOperator::LessThan:
    return comparison < 0;
  case ComparisonOperator::GreaterThanOrEqual:
    return comparison >= 0;
  case ComparisonOperator::LessThanOrEqual:
    return comparison <= 0;
  case ComparisonOperator::In:
  case ComparisonOperator::NotIn:
    throw std::invalid_argument("IN predicates require a value list");
  case ComparisonOperator::Between:
  case ComparisonOperator::NotBetween:
    throw std::invalid_argument("BETWEEN predicates require two range bounds");
  case ComparisonOperator::Like:
  case ComparisonOperator::NotLike:
    throw std::invalid_argument("LIKE predicates require a text pattern");
  }
  throw std::invalid_argument("Unknown WHERE comparison operator");
}

struct CompiledWhereExpression {
  unidb::sql::WhereExpression::Kind kind{
      unidb::sql::WhereExpression::Kind::Predicate};
  std::size_t columnIndex{0};
  WhereEquals predicate;
  std::vector<CompiledWhereExpression> children;
};

CompiledWhereExpression
compileWhereExpression(const WhereExpression &expression,
                       const std::vector<ColumnDefinition> &schema,
                       const std::vector<SqlType> &types) {
  CompiledWhereExpression compiled;
  compiled.kind = expression.kind;
  if (expression.kind == WhereExpression::Kind::Predicate) {
    const auto found =
        std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
          return column.name == expression.predicate.columnName;
        });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown WHERE column: " +
                                  expression.predicate.columnName);
    }
    compiled.columnIndex =
        static_cast<std::size_t>(std::distance(schema.begin(), found));
    const bool isMembership =
        expression.predicate.comparison == ComparisonOperator::In ||
        expression.predicate.comparison == ComparisonOperator::NotIn;
    const bool isRange =
        expression.predicate.comparison == ComparisonOperator::Between ||
        expression.predicate.comparison == ComparisonOperator::NotBetween;
    const bool isLike =
        expression.predicate.comparison == ComparisonOperator::Like ||
        expression.predicate.comparison == ComparisonOperator::NotLike;
    if (isLike && types[compiled.columnIndex] != SqlType::Text) {
      throw std::invalid_argument("LIKE requires a TEXT column: " +
                                  found->name);
    }
    if (!isMembership) {
      const bool valueIsInt =
          std::holds_alternative<std::int64_t>(expression.predicate.value);
      if ((types[compiled.columnIndex] == SqlType::Int) != valueIsInt) {
        throw std::invalid_argument("WHERE value type does not match column: " +
                                    found->name);
      }
    }
    if (isRange) {
      if (!expression.predicate.upperBound.has_value()) {
        throw std::invalid_argument(
            "BETWEEN predicates require two range bounds");
      }
      const bool upperIsInt = std::holds_alternative<std::int64_t>(
          *expression.predicate.upperBound);
      if ((types[compiled.columnIndex] == SqlType::Int) != upperIsInt) {
        throw std::invalid_argument("WHERE value type does not match column: " +
                                    found->name);
      }
    }
    if (isLike &&
        !std::holds_alternative<std::string>(expression.predicate.value)) {
      throw std::invalid_argument("LIKE pattern must be TEXT");
    }
    if (isMembership) {
      if (expression.predicate.values.empty()) {
        throw std::invalid_argument("IN list must contain at least one value");
      }
      for (const auto &value : expression.predicate.values) {
        const bool valueIsInt = std::holds_alternative<std::int64_t>(value);
        if ((types[compiled.columnIndex] == SqlType::Int) != valueIsInt) {
          throw std::invalid_argument(
              "WHERE value type does not match column: " + found->name);
        }
      }
    }
    compiled.predicate = expression.predicate;
    return compiled;
  }

  if (expression.children.empty()) {
    throw std::invalid_argument("WHERE logical expression cannot be empty");
  }
  compiled.children.reserve(expression.children.size());
  for (const auto &child : expression.children) {
    compiled.children.push_back(compileWhereExpression(child, schema, types));
  }
  return compiled;
}

bool matchesWhereExpression(const std::vector<SqlValue> &row,
                            const CompiledWhereExpression &expression) {
  if (expression.kind == WhereExpression::Kind::Predicate) {
    return matchesWhere(row[expression.columnIndex], expression.predicate);
  }
  if (expression.kind == WhereExpression::Kind::And) {
    return std::all_of(
        expression.children.begin(), expression.children.end(),
        [&](const auto &child) { return matchesWhereExpression(row, child); });
  }
  return std::any_of(
      expression.children.begin(), expression.children.end(),
      [&](const auto &child) { return matchesWhereExpression(row, child); });
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
  const auto &join = *select.innerJoin;
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
  const auto &join = *select.innerJoin;
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

  std::optional<CompiledWhereExpression> compiledWhere;
  if (const auto *where = std::get_if<WhereExpression>(&select.where)) {
    const auto qualified = qualifyJoinWhereColumns(
        *where, leftQualifier, leftSchema, rightQualifier, rightSchema);
    compiledWhere =
        compileWhereExpression(qualified, joinedSchema, joinedTypes);
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
      if (compareSqlValues(leftRow[leftKeyIndex], rightRow[rightKeyIndex]) !=
          0) {
        continue;
      }
      std::vector<SqlValue> joinedRow;
      joinedRow.reserve(leftRow.size() + rightRow.size());
      joinedRow.insert(joinedRow.end(), leftRow.begin(), leftRow.end());
      joinedRow.insert(joinedRow.end(), rightRow.begin(), rightRow.end());
      if (compiledWhere.has_value() &&
          !matchesWhereExpression(joinedRow, *compiledWhere)) {
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
      throw std::invalid_argument("UNION ALL requires at least two SELECT branches");
    }

    QueryExecutionResult combined;
    bool firstBranch = true;
    for (const auto &select : unionAll->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      const auto &branchResult =
          std::get<QueryExecutionResult>(branchOutcome);
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
          if (branchResult.columns[index].type != combined.columns[index].type) {
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

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    if (select->innerJoin.has_value()) {
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->innerJoin->tableName);
      schemaTypes(leftSchema);
      schemaTypes(rightSchema);
      validateInnerJoinColumns(*select, leftSchema, rightSchema);

      return executeInnerJoin(rdbServicePort, *select, leftSchema, rightSchema);
    }
    const auto schema = rdbServicePort.getTableDefinition(select->tableName);
    const auto types = schemaTypes(schema);

    const auto *where = std::get_if<WhereExpression>(&select->where);
    std::optional<CompiledWhereExpression> compiledWhere;
    if (where != nullptr) {
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      const auto normalized =
          normalizeSingleTableWhereColumns(*where, tableQualifier);
      compiledWhere = compileWhereExpression(normalized, schema, types);
    }

    std::vector<std::size_t> selectedIndexes;
    std::vector<ColumnDefinition> selectedColumns;
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
      for (const auto &columnName : select->columnNames) {
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
        }
        if (!outputNames.insert(selectedColumns[index].name).second) {
          throw std::invalid_argument("Duplicate SELECT output column name: " +
                                      selectedColumns[index].name);
        }
      }
    }

    std::vector<std::pair<std::size_t, bool>> orderIndexes;
    orderIndexes.reserve(select->orderBy.size());
    const std::string tableQualifier =
        select->tableAlias.value_or(select->tableName);
    for (const auto &order : select->orderBy) {
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
      orderIndexes.emplace_back(orderIndex, order.ascending);
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
      if (select->distinct || !select->orderBy.empty() ||
          select->limit.has_value() || select->offset != 0) {
        throw std::invalid_argument(
            "Grouped aggregate SELECT cannot be combined with DISTINCT, "
            "ORDER BY, LIMIT, or OFFSET");
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
      if (select->distinct || !select->orderBy.empty() ||
          select->limit.has_value() || select->offset != 0) {
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

    const auto encodedRows = rdbServicePort.selectAllRows(select->tableName);
    std::vector<std::vector<SqlValue>> matchedRows;
    matchedRows.reserve(encodedRows.size());
    for (const auto &bytes : encodedRows) {
      auto decoded = SqlTupleCodec::decode(types, bytes);
      if (compiledWhere.has_value() &&
          !matchesWhereExpression(decoded, *compiledWhere)) {
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
              const int comparison =
                  compareSqlValues((*row)[*aggregateIndex], aggregateValue);
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
              compareSqlValues(aggregateValue, select->having->value);
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
          const int comparison = compareSqlValues(row[*aggregateIndex], value);
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

    if (!orderIndexes.empty()) {
      std::stable_sort(matchedRows.begin(), matchedRows.end(),
                       [&](const auto &left, const auto &right) {
                         for (const auto &[index, ascending] : orderIndexes) {
                           const int comparison =
                               compareSqlValues(left[index], right[index]);
                           if (comparison != 0) {
                             return ascending ? comparison < 0 : comparison > 0;
                           }
                         }
                         return false;
                       });
    }

    const std::size_t offset = select->offset > matchedRows.size()
                                   ? matchedRows.size()
                                   : static_cast<std::size_t>(select->offset);
    if (offset > 0) {
      matchedRows.erase(matchedRows.begin(), matchedRows.begin() + offset);
    }
    if (select->limit.has_value() && *select->limit < matchedRows.size()) {
      matchedRows.resize(static_cast<std::size_t>(*select->limit));
    }

    QueryExecutionResult result;
    result.kind = QueryExecutionResultKind::Rows;
    result.message = "Rows selected";
    result.columns = std::move(selectedColumns);
    result.rows.reserve(matchedRows.size());
    for (const auto &decoded : matchedRows) {
      std::vector<SqlValue> projected;
      projected.reserve(selectedIndexes.size());
      for (const auto index : selectedIndexes) {
        projected.push_back(decoded[index]);
      }
      result.rows.push_back(std::move(projected));
    }
    return result;
  }

  return QueryExecutionError{QueryExecutionErrorCode::UnsupportedStatement,
                             "Statement is outside the V1 SQL subset"};
}

} // namespace QueryExecutionService
