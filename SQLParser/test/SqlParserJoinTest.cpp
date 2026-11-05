#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

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
  for (const auto *sql :
       {"SELECT users.id FROM users INNER orders ON users.id = orders.user_id",
        "SELECT users.id FROM users INNER JOIN ON users.id = orders.user_id",
        "SELECT users.id FROM users INNER JOIN orders users.id = orders.user_id",
        "SELECT users.id FROM users INNER JOIN orders ON id = orders.user_id",
        "SELECT users.id FROM users INNER JOIN orders ON users.id = user_id",
        "SELECT users.id FROM users INNER JOIN orders ON users.id > orders.user_id",
        "SELECT users.id FROM users INNER JOIN orders ON users.id =",
        "SELECT users.id FROM users INNER JOIN orders ON users. = orders.user_id"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
