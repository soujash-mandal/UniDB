#pragma once

#include "../port/RdbServicePort.h"
#include "../../SQLParserContract/sqlParser.h"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace QueryExecutionService {

enum class QueryExecutionResultKind { Command, Rows };

struct QueryExecutionResult {
  QueryExecutionResultKind kind;
  std::string message;
  std::vector<unidb::sql::ColumnDefinition> columns;
  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  uint32_t affectedRows = 0;
};

enum class QueryExecutionErrorCode {
  SyntaxError,
  UnsupportedStatement,
  ExecutionError
};

struct QueryExecutionError {
  QueryExecutionErrorCode code;
  std::string message;
};

using QueryExecutionOutcome =
    std::variant<QueryExecutionResult, QueryExecutionError>;

class ExecuteQueryAction {
public:
  ExecuteQueryAction(RdbServicePort &rdbServicePort,
                     const unidb::sql::SqlParser &sqlParser);

  QueryExecutionOutcome execute(std::string_view sql) const;

private:
  QueryExecutionOutcome executeStatement(
      const unidb::sql::SqlStatement &statement) const;

  RdbServicePort &rdbServicePort;
  const unidb::sql::SqlParser &sqlParser;
};

} // namespace QueryExecutionService
