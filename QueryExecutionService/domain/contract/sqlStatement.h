#pragma once

#include <cstdint>
#include <memory>
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

struct ShowTablesStatement {};

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
  NotBetween,
  Like,
  NotLike
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

enum class AggregateFunction { Count, Min, Max, Sum, Avg };

struct AggregateExpression {
  AggregateFunction function;
  std::optional<std::string> columnName;
  std::optional<std::string> alias;
};

struct HavingExpression {
  AggregateExpression aggregate;
  ComparisonOperator comparison{ComparisonOperator::Equal};
  SqlValue value;
};

struct ColumnReference {
  std::optional<std::string> tableName;
  std::string columnName;
};

struct InnerJoinClause {
  std::string tableName;
  std::optional<std::string> tableAlias;
  ColumnReference left;
  ColumnReference right;
  ComparisonOperator comparison{ComparisonOperator::Equal};
};

struct CrossJoinClause {
  std::string tableName;
  std::optional<std::string> tableAlias;
};

struct CaseWhenClause {
  WhereExpression condition;
  SqlValue result;
};

struct CaseExpression {
  std::vector<CaseWhenClause> branches;
  SqlValue elseResult;
};

enum class IntegerBinaryOperator { Add, Subtract, Multiply, Divide };
enum class IntegerUnaryOperator { Positive, Negative };

struct IntegerExpression {
  enum class Kind { Literal, Column, Unary, Binary };
  Kind kind{Kind::Literal};
  std::int64_t literal{0};
  std::string columnName;
  IntegerBinaryOperator operation{IntegerBinaryOperator::Add};
  IntegerUnaryOperator unaryOperation{IntegerUnaryOperator::Positive};
  std::shared_ptr<IntegerExpression> operand;
  std::shared_ptr<IntegerExpression> left;
  std::shared_ptr<IntegerExpression> right;
};

enum class TextFunction { Upper, Lower, Length };

struct TextFunctionExpression {
  TextFunction function{TextFunction::Upper};
  std::string columnName;
};

struct SelectStatement {
  // When selectAll is true, columnNames is ignored.
  bool selectAll{false};
  std::vector<std::string> columnNames;
  // Parallel to columnNames; nullopt means no explicit alias.
  std::vector<std::optional<std::string>> columnAliases;
  // Parallel to columnNames; a value means the item is a CASE expression.
  std::vector<std::optional<CaseExpression>> caseExpressions;
  // Parallel to columnNames; populated for computed integer expressions.
  std::vector<std::optional<IntegerExpression>> integerExpressions;
  // Parallel to columnNames; populated for scalar TEXT function projections.
  std::vector<std::optional<TextFunctionExpression>> textFunctions;
  bool distinct{false};
  std::optional<AggregateExpression> aggregate;
  std::vector<std::string> groupBy;
  std::optional<HavingExpression> having;
  std::string tableName;
  std::optional<std::string> tableAlias;
  std::vector<InnerJoinClause> innerJoins;
  std::optional<CrossJoinClause> crossJoin;
  // Expression tree preserves parentheses and SQL boolean precedence.
  std::variant<std::monostate, WhereExpression> where;
  std::vector<OrderByColumn> orderBy;
  std::optional<std::uint64_t> limit;
  std::uint64_t offset{0};
};

struct UnionAllStatement {
  // Each branch is evaluated independently; rows are concatenated in order.
  std::vector<SelectStatement> selects;
};

struct UnionStatement {
  // Each branch is evaluated independently; duplicate rows are removed.
  std::vector<SelectStatement> selects;
};

using SqlStatement =
    std::variant<CreateTableStatement, InsertStatement, ShowTablesStatement,
                 SelectStatement,
                 UnionAllStatement, UnionStatement>;

} // namespace unidb::sql
