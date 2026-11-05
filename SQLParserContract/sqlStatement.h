#pragma once

#include <cstdint>
#include <optional>
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

enum class ComparisonOperator {
  Equal,
  NotEqual,
  GreaterThan,
  LessThan,
  GreaterThanOrEqual,
  LessThanOrEqual
};

struct WhereEquals {
  std::string columnName;
  SqlValue value;
  ComparisonOperator comparison{ComparisonOperator::Equal};
};

struct WhereExpression {
  // Each inner group is joined by AND; groups are joined by OR.
  // This representation encodes SQL's AND-before-OR precedence.
  std::vector<std::vector<WhereEquals>> disjunctions;
};

struct OrderByColumn {
  std::string columnName;
  bool ascending{true};
};

struct SelectStatement {
  // When selectAll is true, columnNames is ignored.
  bool selectAll{false};
  std::vector<std::string> columnNames;
  std::string tableName;
  // AND binds more tightly than OR. Parenthesized expressions are not yet
  // supported.
  std::variant<std::monostate, WhereExpression> where;
  std::vector<OrderByColumn> orderBy;
  std::optional<std::uint64_t> limit;
  std::uint64_t offset{0};
};

using SqlStatement =
    std::variant<CreateTableStatement, InsertStatement, SelectStatement>;

} // namespace unidb::sql
