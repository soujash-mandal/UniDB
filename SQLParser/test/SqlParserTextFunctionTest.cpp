#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserTextFunctionTest, ParsesSupportedTextFunctionsAndAliases) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT UPPER(name) AS upper_name, LOWER(users.name), "
                   "LENGTH(name) AS name_size FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.textFunctions.size(), 3U);
  ASSERT_TRUE(select.textFunctions[0].has_value());
  EXPECT_EQ(select.textFunctions[0]->function, TextFunction::Upper);
  EXPECT_EQ(select.textFunctions[0]->columnName, "name");
  ASSERT_TRUE(select.columnAliases[0].has_value());
  EXPECT_EQ(*select.columnAliases[0], "upper_name");

  ASSERT_TRUE(select.textFunctions[1].has_value());
  EXPECT_EQ(select.textFunctions[1]->function, TextFunction::Lower);
  EXPECT_EQ(select.textFunctions[1]->columnName, "users.name");

  ASSERT_TRUE(select.textFunctions[2].has_value());
  EXPECT_EQ(select.textFunctions[2]->function, TextFunction::Length);
  EXPECT_EQ(select.textFunctions[2]->columnName, "name");
  ASSERT_TRUE(select.columnAliases[2].has_value());
  EXPECT_EQ(*select.columnAliases[2], "name_size");
}

TEST(SqlParserTextFunctionTest, RejectsMalformedFunctionArguments) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "SELECT UPPER() FROM users",
           "SELECT LOWER(name FROM users",
           "SELECT LENGTH(1) FROM users",
           "SELECT UPPER(name, id) FROM users",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
