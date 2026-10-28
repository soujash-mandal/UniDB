#include "ExecuteQueryAction.h"

#include "../adapter/SqlTupleCodec.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService {
namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::CreateTableStatement;
using unidb::sql::InsertStatement;
using unidb::sql::SelectStatement;
using unidb::sql::SqlStatement;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;

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
    if (insert->columnNames.size() != insert->values.size()) {
      throw std::invalid_argument("INSERT column and value counts must match");
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
          std::holds_alternative<std::int64_t>(insert->values[inputIndex]);
      if ((types[schemaIndex] == SqlType::Int) != valueIsInt) {
        throw std::invalid_argument(
            "INSERT value type does not match column: " + found->name);
      }
      orderedValues[schemaIndex] = insert->values[inputIndex];
      assigned[schemaIndex] = true;
    }

    if (std::find(assigned.begin(), assigned.end(), false) != assigned.end()) {
      throw std::invalid_argument(
          "V1 INSERT requires a value for every table column");
    }

    const auto bytes = SqlTupleCodec::encode(types, orderedValues);
    rdbServicePort.insertRow(insert->tableName, bytes);
    return commandResult("Row inserted", 1);
  }

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    if (!select->selectAll ||
        !std::holds_alternative<std::monostate>(select->where)) {
      return QueryExecutionError{QueryExecutionErrorCode::UnsupportedStatement,
                                 "V1 supports only SELECT * FROM table"};
    }

    const auto schema = rdbServicePort.getTableDefinition(select->tableName);
    const auto types = schemaTypes(schema);
    const auto encodedRows = rdbServicePort.selectAllRows(select->tableName);

    QueryExecutionResult result;
    result.kind = QueryExecutionResultKind::Rows;
    result.message = "Rows selected";
    result.columns = schema;
    result.rows.reserve(encodedRows.size());
    for (const auto &bytes : encodedRows) {
      result.rows.push_back(SqlTupleCodec::decode(types, bytes));
    }
    return result;
  }

  return QueryExecutionError{QueryExecutionErrorCode::UnsupportedStatement,
                             "Statement is outside the V1 SQL subset"};
}

} // namespace QueryExecutionService
