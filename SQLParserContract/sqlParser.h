#pragma once

#include "sqlStatement.h"

#include <string>
#include <string_view>
#include <variant>

namespace unidb::sql {

enum class SqlParseErrorCode { SyntaxError, UnsupportedStatement };

struct SqlParseError {
  SqlParseErrorCode code;
  std::string message;
};

using SqlParseResult = std::variant<SqlStatement, SqlParseError>;

class SqlParser {
public:
  virtual ~SqlParser() = default;

  // Parses SQL into a typed statement or returns a syntax/unsupported error.
  // This interface does not execute statements or access database storage.
  virtual SqlParseResult parse(std::string_view sql) const = 0;
};

} // namespace unidb::sql
