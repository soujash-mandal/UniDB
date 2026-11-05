#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace unidb::sql {
namespace {

TEST(SqlParserTest, ParsesOrderByWithDefaultAscendingAndExplicitDirections) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id FROM users ORDER BY name, id DESC;");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.orderBy.size(), 2U);
  EXPECT_EQ(select.orderBy[0].columnName, "name");
  EXPECT_TRUE(select.orderBy[0].ascending);
  EXPECT_EQ(select.orderBy[1].columnName, "id");
  EXPECT_FALSE(select.orderBy[1].ascending);
}

TEST(SqlParserTest, ParsesWhereBeforeOrderBy) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id FROM users WHERE id > 1 ORDER BY name DESC");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
  ASSERT_EQ(select.orderBy.size(), 1U);
  EXPECT_EQ(select.orderBy[0].columnName, "name");
  EXPECT_FALSE(select.orderBy[0].ascending);
}

TEST(SqlParserTest, RejectsMalformedOrderByClauses) {
  SqlParserImpl parser;
  for (const auto *sql :
       {"SELECT * FROM users ORDER", "SELECT * FROM users ORDER id",
        "SELECT * FROM users ORDER BY", "SELECT * FROM users ORDER BY id,",
        "SELECT * FROM users ORDER BY id ASC DESC"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, ParsesLimitAndOffsetAfterOrderBy) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id FROM users ORDER BY id DESC LIMIT 5 OFFSET 2");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(select.limit.has_value());
  EXPECT_EQ(*select.limit, 5U);
  EXPECT_EQ(select.offset, 2U);
  ASSERT_EQ(select.orderBy.size(), 1U);
  EXPECT_FALSE(select.orderBy[0].ascending);
}

TEST(SqlParserTest, ParsesLimitWithoutOffsetAndOffsetWithoutLimit) {
  SqlParserImpl parser;
  const auto limitResult = parser.parse("SELECT * FROM users LIMIT 0");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(limitResult));
  const auto &limited =
      std::get<SelectStatement>(std::get<SqlStatement>(limitResult));
  ASSERT_TRUE(limited.limit.has_value());
  EXPECT_EQ(*limited.limit, 0U);
  EXPECT_EQ(limited.offset, 0U);

  const auto offsetResult = parser.parse("SELECT * FROM users OFFSET 3");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(offsetResult));
  const auto &offset =
      std::get<SelectStatement>(std::get<SqlStatement>(offsetResult));
  EXPECT_FALSE(offset.limit.has_value());
  EXPECT_EQ(offset.offset, 3U);
}

TEST(SqlParserTest, RejectsMalformedLimitAndOffsetClauses) {
  SqlParserImpl parser;
  for (const auto *sql :
       {"SELECT * FROM users LIMIT", "SELECT * FROM users LIMIT -1",
        "SELECT * FROM users LIMIT '2'",
        "SELECT * FROM users LIMIT 18446744073709551616",
        "SELECT * FROM users LIMIT 1 LIMIT 2",
        "SELECT * FROM users OFFSET 1 OFFSET 2",
        "SELECT * FROM users LIMIT 1 OFFSET"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

} // namespace
} // namespace unidb::sql
