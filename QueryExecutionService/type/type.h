#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace unidb::sql {

enum class SqlType { Int, Text };

using SqlValue = std::variant<std::int64_t, std::string>;

struct ColumnDefinition {
  std::string name;
  SqlType type;
};

} // namespace unidb::sql

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
