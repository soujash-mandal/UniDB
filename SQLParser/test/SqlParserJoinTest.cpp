#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

namespace unidb::sql {
namespace {

TEST(SqlParserJoinTest, ParsesInnerJoinWithQualifiedOnColumns) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT users.id, orders.amount FROM users "
                   "INNER JOIN orders ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.columnNames.size(), 2U);
  EXPECT_EQ(select.columnNames[0], "users.id");
  EXPECT_EQ(select.columnNames[1], "orders.amount");
  ASSERT_TRUE(select.innerJoin.has_value());
  EXPECT_EQ(select.innerJoin->tableName, "orders");
  ASSERT_TRUE(select.innerJoin->left.tableName.has_value());
  EXPECT_EQ(*select.innerJoin->left.tableName, "users");
  EXPECT_EQ(select.innerJoin->left.columnName, "id");
  ASSERT_TRUE(select.innerJoin->right.tableName.has_value());
  EXPECT_EQ(*select.innerJoin->right.tableName, "orders");
  EXPECT_EQ(select.innerJoin->right.columnName, "user_id");
}

TEST(SqlParserJoinTest, ParsesInnerJoinBeforeWhereClause) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT users.name FROM users INNER JOIN orders "
                   "ON users.id = orders.user_id WHERE id > 10");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(select.innerJoin.has_value());
  EXPECT_TRUE(std::holds_alternative<WhereExpression>(select.where));
}

TEST(SqlParserJoinTest, RejectsMalformedInnerJoinClauses) {
  SqlParserImpl parser;
  const std::vector<std::string> invalidQueries{
      "SELECT a.x FROM a INNER b ON a.x = b.y",
      "SELECT a.x FROM a INNER JOIN ON a.x = b.y",
      "SELECT a.x FROM a INNER JOIN b a.x = b.y",
      "SELECT a.x FROM a INNER JOIN b ON x = b.y",
      "SELECT a.x FROM a INNER JOIN b ON a.x = y",

      "SELECT a.x FROM a INNER JOIN b ON a.x =",
      "SELECT a.x FROM a INNER JOIN b ON a. = b.y"};
  for (const auto &sql : invalidQueries) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

TEST(SqlParserJoinTest, ParsesNonEqualityJoinComparisons) {
  SqlParserImpl parser;
  const std::vector<std::pair<std::string, ComparisonOperator>> cases{
      {">", ComparisonOperator::GreaterThan},
      {"<", ComparisonOperator::LessThan},
      {">=", ComparisonOperator::GreaterThanOrEqual},
      {"<=", ComparisonOperator::LessThanOrEqual},
      {"!=", ComparisonOperator::NotEqual}};
  for (const auto &[op, expected] : cases) {
    const auto result = parser.parse("SELECT a.id FROM a INNER JOIN b ON a.id " + op + " b.id");
    ASSERT_TRUE(std::holds_alternative<SqlStatement>(result)) << op;
    const auto &select = std::get<SelectStatement>(std::get<SqlStatement>(result));
    ASSERT_TRUE(select.innerJoin.has_value());
    EXPECT_EQ(select.innerJoin->comparison, expected);
  }
}

TEST(SqlParserJoinTest, ParsesTableAliasesWithAndWithoutAs) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT u.id FROM users AS u INNER JOIN orders o "
                   "ON u.id = o.user_id");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(select.tableAlias.has_value());
  EXPECT_EQ(*select.tableAlias, "u");
  ASSERT_TRUE(select.innerJoin.has_value());
  ASSERT_TRUE(select.innerJoin->tableAlias.has_value());
  EXPECT_EQ(*select.innerJoin->tableAlias, "o");
  ASSERT_TRUE(select.innerJoin->left.tableName.has_value());
  EXPECT_EQ(*select.innerJoin->left.tableName, "u");
  ASSERT_TRUE(select.innerJoin->right.tableName.has_value());
  EXPECT_EQ(*select.innerJoin->right.tableName, "o");
}

TEST(SqlParserJoinTest, ParsesQualifiedOrderByColumn) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT u.id FROM users u ORDER BY u.id DESC");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.orderBy.size(), 1U);
  EXPECT_EQ(select.orderBy[0].columnName, "u.id");
  EXPECT_FALSE(select.orderBy[0].ascending);
}

TEST(SqlParserJoinTest, RejectsMissingAliasAfterAs) {
  SqlParserImpl parser;
  for (const auto *sql :
       {"SELECT * FROM users AS",
        "SELECT * FROM users INNER JOIN orders AS ON users.id = orders.id"}) {
    EXPECT_TRUE(std::holds_alternative<SqlParseError>(parser.parse(sql)))
        << sql;
  }
}

} // namespace
} // namespace unidb::sql
