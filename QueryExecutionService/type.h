#pragma once

#include "domain/contract/sqlParser.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace QueryExecutionService {

enum class QueryExecutionResultKind { Command, Rows };

struct QueryExecutionResult {
  QueryExecutionResultKind kind;
  std::string message;
  std::vector<unidb::sql::ColumnDefinition> columns;
  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  std::uint32_t affectedRows = 0;
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

struct ResolvedJoinColumn {
  const unidb::sql::ColumnDefinition *column;
  std::string tableName;
  std::size_t index;
};

} // namespace QueryExecutionService
