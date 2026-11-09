#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserBooleanTest, ParsesTrueAndFalseAsDistinctBooleanValues) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO flags (enabled) VALUES (TRUE), (false)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 2U);
  ASSERT_EQ(insert.rows[0].size(), 1U);
  ASSERT_EQ(insert.rows[1].size(), 1U);
  EXPECT_TRUE(std::get<bool>(insert.rows[0][0]));
  EXPECT_FALSE(std::get<bool>(insert.rows[1][0]));
}

TEST(SqlParserBooleanTest, KeepsNullOutsideTheSupportedLiteralSubset) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO flags (enabled) VALUES (NULL)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::UnsupportedStatement);
}

} // namespace
} // namespace unidb::sql
