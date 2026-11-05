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

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    const auto schema = rdbServicePort.getTableDefinition(select->tableName);
    const auto types = schemaTypes(schema);

    const auto *where = std::get_if<WhereExpression>(&select->where);
    std::optional<CompiledWhereExpression> compiledWhere;
    if (where != nullptr) {
      compiledWhere = compileWhereExpression(*where, schema, types);
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
      for (const auto &columnName : select->columnNames) {
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == columnName;
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
    for (const auto &order : select->orderBy) {
      const auto found =
          std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == order.columnName;
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
        const auto found =
            std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
              return column.name == groupColumn;
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
