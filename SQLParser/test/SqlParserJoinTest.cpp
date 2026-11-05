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
      "SELECT a.x FROM a INNER JOIN b ON a.x > b.y",
      "SELECT a.x FROM a INNER JOIN b ON a.x =",
      "SELECT a.x FROM a INNER JOIN b ON a. = b.y"};
  for (const auto &sql : invalidQueries) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
