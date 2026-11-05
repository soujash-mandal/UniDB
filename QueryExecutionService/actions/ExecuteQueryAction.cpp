#include "ExecuteQueryAction.h"

#include "../adapter/SqlTupleCodec.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService {
namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::ComparisonOperator;
using unidb::sql::CreateTableStatement;
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

bool matchesWhere(const SqlValue &rowValue, const WhereEquals &predicate) {
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
  }
  throw std::invalid_argument("Unknown WHERE comparison operator");
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
        throw std::invalid_argument("INSERT column and value counts must match");
      }

      std::vector<SqlValue> orderedValues(schema.size());
      std::vector<bool> assigned(schema.size(), false);
      for (std::size_t inputIndex = 0;
           inputIndex < insert->columnNames.size(); ++inputIndex) {
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
        rdbServicePort.insertRow(insert->tableName, bytes);
        ++insertedRows;
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

    using CompiledPredicate = std::pair<std::size_t, WhereEquals>;
    std::vector<std::vector<CompiledPredicate>> compiledWhere;
    const auto *where = std::get_if<WhereExpression>(&select->where);
    if (where != nullptr) {
      compiledWhere.reserve(where->disjunctions.size());
      for (const auto &conjunction : where->disjunctions) {
        auto &compiledConjunction = compiledWhere.emplace_back();
        compiledConjunction.reserve(conjunction.size());
        for (const auto &predicate : conjunction) {
          const auto found = std::find_if(
              schema.begin(), schema.end(), [&](const auto &column) {
                return column.name == predicate.columnName;
              });
          if (found == schema.end()) {
            throw std::invalid_argument("Unknown WHERE column: " +
                                        predicate.columnName);
          }
          const auto filterIndex =
              static_cast<std::size_t>(std::distance(schema.begin(), found));
          const bool valueIsInt =
              std::holds_alternative<std::int64_t>(predicate.value);
          if ((types[filterIndex] == SqlType::Int) != valueIsInt) {
            throw std::invalid_argument(
                "WHERE value type does not match column: " + found->name);
          }
          compiledConjunction.emplace_back(filterIndex, predicate);
        }
      }
    }

    std::vector<std::size_t> selectedIndexes;
    std::vector<ColumnDefinition> selectedColumns;
    if (select->selectAll) {
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
      orderIndexes.emplace_back(
          static_cast<std::size_t>(std::distance(schema.begin(), found)),
          order.ascending);
    }

    const auto encodedRows = rdbServicePort.selectAllRows(select->tableName);
    std::vector<std::vector<SqlValue>> matchedRows;
    matchedRows.reserve(encodedRows.size());
    for (const auto &bytes : encodedRows) {
      auto decoded = SqlTupleCodec::decode(types, bytes);
      if (where != nullptr) {
        const bool matchesExpression = std::any_of(
            compiledWhere.begin(), compiledWhere.end(),
            [&](const auto &conjunction) {
              return std::all_of(conjunction.begin(), conjunction.end(),
                                 [&](const auto &compiled) {
                                   return matchesWhere(decoded[compiled.first],
                                                       compiled.second);
                                 });
            });
        if (!matchesExpression) {
          continue;
        }
      }
      matchedRows.push_back(std::move(decoded));
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
