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
  std::vector<std::vector<SqlValue>> rows;
};

enum class ComparisonOperator {
  Equal,
  NotEqual,
  GreaterThan,
  LessThan,
  GreaterThanOrEqual,
  LessThanOrEqual,
  In,
  NotIn,
  Between,
  NotBetween
};

struct WhereEquals {
  std::string columnName;
  SqlValue value;
  std::optional<SqlValue> upperBound;
  std::vector<SqlValue> values;
  ComparisonOperator comparison{ComparisonOperator::Equal};
};

struct WhereExpression {
  enum class Kind { Predicate, And, Or };

  Kind kind{Kind::Predicate};
  WhereEquals predicate;
  std::vector<WhereExpression> children;
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
  // Expression tree preserves parentheses and SQL boolean precedence.
  std::variant<std::monostate, WhereExpression> where;
  std::vector<OrderByColumn> orderBy;
  std::optional<std::uint64_t> limit;
  std::uint64_t offset{0};
};

using SqlStatement =
    std::variant<CreateTableStatement, InsertStatement, SelectStatement>;

} // namespace unidb::sql
