#include "ExecuteQueryAction.h"

#include "../domain/codec/SqlTupleCodec.h"
#include "../domain/join/JoinExecutor.h"
#include "../domain/join/JoinValidator.h"
#include "../domain/planning/SqlSelectPlanner.h"
#include "../domain/planning/SqlSelectResultProcessor.h"
#include "../domain/planning/SqlStatementRules.h"
#include "../domain/schema/SqlSchemaResolver.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace QueryExecutionService {
namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::CreateTableStatement;
using unidb::sql::InsertStatement;
using unidb::sql::SelectStatement;
using unidb::sql::SqlStatement;
using unidb::sql::UnionAllStatement;
using unidb::sql::UnionStatement;

QueryExecutionError executionError(std::string message) {
  return {QueryExecutionErrorCode::ExecutionError, std::move(message)};
}

// Keep RDB access and tuple decoding in the Action; JOIN domain code only
// receives schemas and in-memory SQL values.
std::vector<std::vector<unidb::sql::SqlValue>>
readRows(RdbServicePort &port, const std::string &tableName,
         const std::vector<unidb::sql::ColumnDefinition> &schema) {
  const auto encodedRows = port.selectAllRows(tableName);
  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  rows.reserve(encodedRows.size());
  for (const auto &bytes : encodedRows) {
    rows.push_back(SqlTupleCodec::decode(schema, bytes));
  }
  return rows;
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
    domain::validateCreateTable(*create);
    rdbServicePort.createTable(*create);
    return commandResult("Table created");
  }

  if (const auto *insert = std::get_if<InsertStatement>(&statement)) {
    const auto schema = rdbServicePort.getTableDefinition(insert->tableName);
    const auto encodedRows = domain::encodeInsertRows(*insert, schema);

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
    std::vector<QueryExecutionResult> branchResults;
    branchResults.reserve(unionAll->selects.size());
    for (const auto &select : unionAll->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      branchResults.push_back(std::get<QueryExecutionResult>(branchOutcome));
    }
    return domain::combineUnionResults(branchResults, false);
  }

  if (const auto *unionDistinct = std::get_if<UnionStatement>(&statement)) {
    std::vector<QueryExecutionResult> branchResults;
    branchResults.reserve(unionDistinct->selects.size());
    for (const auto &select : unionDistinct->selects) {
      const auto branchOutcome = executeStatement(SqlStatement{select});
      if (const auto *error =
              std::get_if<QueryExecutionError>(&branchOutcome)) {
        return *error;
      }
      branchResults.push_back(std::get<QueryExecutionResult>(branchOutcome));
    }
    return domain::combineUnionResults(branchResults, true);
  }

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    SelectStatement executableSelect = *select;
    std::optional<std::vector<ColumnDefinition>> subqueryOuterSchema;
    if (auto *where = std::get_if<unidb::sql::WhereExpression>(
            &executableSelect.where)) {
      std::function<bool(const unidb::sql::WhereExpression &)> containsSubquery =
          [&](const auto &expression) {
            if (expression.kind ==
                    unidb::sql::WhereExpression::Kind::Predicate &&
                expression.predicate.subquery != nullptr) {
              return true;
            }
            return std::any_of(expression.children.begin(),
                               expression.children.end(), containsSubquery);
          };
      if (containsSubquery(*where)) {
        if (!select->innerJoins.empty() || select->leftJoin.has_value() ||
            select->rightJoin.has_value() || select->fullJoin.has_value() ||
            select->crossJoin.has_value()) {
          throw std::invalid_argument(
              "IN subqueries are supported only in single-table SELECT queries");
        }
        subqueryOuterSchema =
            rdbServicePort.getTableDefinition(select->tableName);
        const auto qualifier = select->tableAlias.value_or(select->tableName);
        std::function<void(unidb::sql::WhereExpression &)> materialize =
            [&](auto &expression) {
              if (expression.kind !=
                  unidb::sql::WhereExpression::Kind::Predicate) {
                for (auto &child : expression.children) {
                  materialize(child);
                }
                return;
              }
              auto &predicate = expression.predicate;
              if (predicate.subquery == nullptr) {
                return;
              }
              std::string outerColumnName = predicate.columnName;
              const auto separator = outerColumnName.find('.');
              if (separator != std::string::npos) {
                if (outerColumnName.find('.', separator + 1) !=
                        std::string::npos ||
                    outerColumnName.substr(0, separator) != qualifier) {
                  throw std::invalid_argument(
                      "Unknown WHERE table reference in IN subquery: " +
                      outerColumnName.substr(0, separator));
                }
                outerColumnName = outerColumnName.substr(separator + 1);
              }
              const auto outerColumn = std::find_if(
                  subqueryOuterSchema->begin(), subqueryOuterSchema->end(),
                  [&](const auto &column) {
                    return column.name == outerColumnName;
                  });
              if (outerColumn == subqueryOuterSchema->end()) {
                throw std::invalid_argument("Unknown WHERE column: " +
                                            outerColumnName);
              }
              const auto subqueryOutcome =
                  executeStatement(SqlStatement{*predicate.subquery});
              if (const auto *error =
                      std::get_if<QueryExecutionError>(&subqueryOutcome)) {
                throw std::invalid_argument("IN subquery failed: " +
                                            error->message);
              }
              const auto &subqueryResult =
                  std::get<QueryExecutionResult>(subqueryOutcome);
              if (subqueryResult.kind != QueryExecutionResultKind::Rows) {
                throw std::invalid_argument("IN subquery must return rows");
              }
              if (subqueryResult.columns.size() != 1U) {
                throw std::invalid_argument(
                    "IN subquery must return exactly one column");
              }
              const auto &innerColumn = subqueryResult.columns.front();
              if (outerColumn->type != innerColumn.type ||
                  (outerColumn->declaredType.has_value() !=
                   innerColumn.declaredType.has_value()) ||
                  (outerColumn->declaredType.has_value() &&
                   outerColumn->declaredType->id !=
                       innerColumn.declaredType->id)) {
                throw std::invalid_argument(
                    "IN subquery column type does not match WHERE column: " +
                    outerColumnName);
              }
              predicate.values.clear();
              predicate.values.reserve(subqueryResult.rows.size());
              for (const auto &row : subqueryResult.rows) {
                if (row.size() != 1U) {
                  throw std::invalid_argument(
                      "IN subquery returned a row with an invalid column count");
                }
                predicate.values.push_back(row.front());
              }
              predicate.subquery.reset();
            };
        materialize(*where);
      }
    }

    if (select->fullJoin.has_value()) {
      if (!select->innerJoins.empty() || select->leftJoin.has_value() ||
          select->rightJoin.has_value() || select->crossJoin.has_value()) {
        throw std::invalid_argument(
            "Mixing FULL JOIN with other JOIN types is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->fullJoin->tableName);
      domain::validateFullJoinColumns(*select, leftSchema, rightSchema);
      const auto leftRows =
          readRows(rdbServicePort, select->tableName, leftSchema);
      const auto rightRows =
          readRows(rdbServicePort, select->fullJoin->tableName, rightSchema);
      return domain::executeFullJoin(*select, leftSchema, rightSchema, leftRows,
                                     rightRows);
    }
    if (select->rightJoin.has_value()) {
      if (!select->innerJoins.empty() || select->leftJoin.has_value() ||
          select->crossJoin.has_value()) {
        throw std::invalid_argument(
            "Mixing RIGHT JOIN with other JOIN types is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->rightJoin->tableName);
      domain::validateRightJoinColumns(*select, leftSchema, rightSchema);
      const auto leftRows =
          readRows(rdbServicePort, select->tableName, leftSchema);
      const auto rightRows =
          readRows(rdbServicePort, select->rightJoin->tableName, rightSchema);
      return domain::executeRightJoin(*select, leftSchema, rightSchema,
                                      leftRows, rightRows);
    }
    if (select->leftJoin.has_value()) {
      if (!select->innerJoins.empty() || select->crossJoin.has_value()) {
        throw std::invalid_argument(
            "Mixing LEFT JOIN with other JOIN types is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->leftJoin->tableName);
      domain::validateLeftJoinColumns(*select, leftSchema, rightSchema);
      const auto leftRows =
          readRows(rdbServicePort, select->tableName, leftSchema);
      const auto rightRows =
          readRows(rdbServicePort, select->leftJoin->tableName, rightSchema);
      return domain::executeLeftJoin(*select, leftSchema, rightSchema, leftRows,
                                     rightRows);
    }
    if (select->leftJoin.has_value()) {
      throw std::invalid_argument("LEFT JOIN parsing is available, but "
                                  "execution is not implemented yet");
    }
    if (select->crossJoin.has_value()) {
      if (!select->innerJoins.empty()) {
        throw std::invalid_argument(
            "Mixing CROSS JOIN and INNER JOIN is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->crossJoin->tableName);
      const auto leftRows =
          readRows(rdbServicePort, select->tableName, leftSchema);
      const auto rightRows =
          readRows(rdbServicePort, select->crossJoin->tableName, rightSchema);
      return domain::executeCrossJoin(*select, leftSchema, rightSchema,
                                      leftRows, rightRows);
    }
    if (!select->innerJoins.empty()) {
      std::vector<std::vector<ColumnDefinition>> schemas;
      schemas.push_back(rdbServicePort.getTableDefinition(select->tableName));
      for (const auto &join : select->innerJoins)
        schemas.push_back(rdbServicePort.getTableDefinition(join.tableName));
      std::vector<std::vector<std::vector<unidb::sql::SqlValue>>> rowsByTable;
      rowsByTable.reserve(schemas.size());
      for (std::size_t index = 0; index < schemas.size(); ++index) {
        const std::string &tableName =
            index == 0 ? select->tableName
                       : select->innerJoins[index - 1].tableName;
        rowsByTable.push_back(
            readRows(rdbServicePort, tableName, schemas[index]));
      }
      if (select->innerJoins.size() == 1) {
        domain::validateInnerJoinColumns(*select, schemas[0], schemas[1]);
        return domain::executeInnerJoin(*select, schemas[0], schemas[1],
                                        rowsByTable[0], rowsByTable[1]);
      }
      return domain::executeMultipleInnerJoins(*select, schemas, rowsByTable);
    }
    const auto schema =
        subqueryOuterSchema.has_value()
            ? *subqueryOuterSchema
            : rdbServicePort.getTableDefinition(select->tableName);
    const auto plan = domain::buildSelectPlan(executableSelect, schema);
    const auto encodedRows = rdbServicePort.selectAllRows(select->tableName);
    auto matchedRows = domain::decodeAndFilterRows(encodedRows, plan.schema,
                                                   plan.compiledWhere);
    return domain::processSelectRows(executableSelect, plan,
                                     std::move(matchedRows));
  }

  return QueryExecutionError{QueryExecutionErrorCode::UnsupportedStatement,
                             "Statement is outside the V1 SQL subset"};
}

} // namespace QueryExecutionService
