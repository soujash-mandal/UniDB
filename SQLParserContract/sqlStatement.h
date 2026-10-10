#pragma once

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

struct CreateTableStatement {
  std::string tableName;
  std::vector<ColumnDefinition> columns;
};

struct InsertStatement {
  std::string tableName;
  std::vector<std::string> columnNames;
  std::vector<SqlValue> values;
};

struct WhereEquals {
  std::string columnName;
  SqlValue value;
};

struct SelectStatement {
  // When selectAll is true, columnNames is ignored.
  bool selectAll{false};
  std::vector<std::string> columnNames;
  std::string tableName;
  // V1 supports at most one equality predicate.
  std::variant<std::monostate, WhereEquals> where;
};

using SqlStatement =
    std::variant<CreateTableStatement, InsertStatement, SelectStatement>;

}  // namespace unidb::sql
