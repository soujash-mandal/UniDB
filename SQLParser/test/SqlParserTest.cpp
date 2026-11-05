#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace unidb::sql {
namespace {

TEST(SqlParserTest, ParsesCreateTableCaseInsensitively) {
  SqlParserImpl parser;
  const auto result = parser.parse("create TABLE users (id INT, name text);");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &statement = std::get<SqlStatement>(result);
  ASSERT_TRUE(std::holds_alternative<CreateTableStatement>(statement));
  const auto &create = std::get<CreateTableStatement>(statement);
  EXPECT_EQ(create.tableName, "users");
  ASSERT_EQ(create.columns.size(), 2U);
  EXPECT_EQ(create.columns[0].name, "id");
  EXPECT_EQ(create.columns[0].type, SqlType::Int);
  EXPECT_EQ(create.columns[1].type, SqlType::Text);
}

TEST(SqlParserTest, ParsesInsertWithSignedIntegerAndEscapedText) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO users (id, name) VALUES (-42, 'O''Reilly')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.values.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(insert.values[0]), -42);
  EXPECT_EQ(std::get<std::string>(insert.values[1]), "O'Reilly");
}

TEST(SqlParserTest, ParsesSelectAllAndOptionalEqualityPredicate) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT * FROM users WHERE id = 7;");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  EXPECT_TRUE(select.selectAll);
  EXPECT_EQ(select.tableName, "users");
  ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
  const auto &where = std::get<WhereExpression>(select.where);
  ASSERT_EQ(where.disjunctions.size(), 1U);
  ASSERT_EQ(where.disjunctions[0].size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(where.disjunctions[0][0].value), 7);
}

TEST(SqlParserTest, ParsesSelectedColumns) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT id, name FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  EXPECT_FALSE(select.selectAll);
  ASSERT_EQ(select.columnNames.size(), 2U);
  EXPECT_EQ(select.columnNames[0], "id");
  EXPECT_EQ(select.columnNames[1], "name");
}

TEST(SqlParserTest, ReportsMalformedSyntax) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, ReportsUnsupportedStatementAndOperators) {
  SqlParserImpl parser;
  const auto join = parser.parse("SELECT * FROM users JOIN roles");
  ASSERT_TRUE(std::holds_alternative<SqlParseError>(join));
  EXPECT_EQ(std::get<SqlParseError>(join).code,
            SqlParseErrorCode::UnsupportedStatement);

  const auto comparison = parser.parse("SELECT * FROM users WHERE id > 2");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(comparison));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(comparison));
  ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
  EXPECT_EQ(
      std::get<WhereExpression>(select.where).disjunctions[0][0].comparison,
      ComparisonOperator::GreaterThan);
}

TEST(SqlParserTest, RejectsMismatchedInsertColumnAndValueCounts) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO users (id, name) VALUES (1)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, RejectsUnsupportedLiteralTypes) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO users (active) VALUES (TRUE)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::UnsupportedStatement);
}

TEST(SqlParserTest, ParsesEverySupportedWhereComparisonOperator) {
  SqlParserImpl parser;
  const std::vector<std::pair<std::string, ComparisonOperator>> cases{
      {"=", ComparisonOperator::Equal},
      {"!=", ComparisonOperator::NotEqual},
      {">", ComparisonOperator::GreaterThan},
      {"<", ComparisonOperator::LessThan},
      {">=", ComparisonOperator::GreaterThanOrEqual},
      {"<=", ComparisonOperator::LessThanOrEqual}};

  for (const auto &[token, expected] : cases) {
    const auto result =
        parser.parse("SELECT id FROM users WHERE id " + token + " 7;");
    ASSERT_TRUE(std::holds_alternative<SqlStatement>(result))
        << "operator: " << token;
    const auto &select =
        std::get<SelectStatement>(std::get<SqlStatement>(result));
    ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
    EXPECT_EQ(
        std::get<WhereExpression>(select.where).disjunctions[0][0].comparison,
        expected)
        << "operator: " << token;
  }
}

TEST(SqlParserTest, RejectsIncompleteWhereComparison) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT * FROM users WHERE id >");
  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, ParsesAndBeforeOrPrecedence) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT id FROM users WHERE id = 1 OR id = 2 AND name = 'B'");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  const auto &where = std::get<WhereExpression>(select.where);
  ASSERT_EQ(where.disjunctions.size(), 2U);
  ASSERT_EQ(where.disjunctions[0].size(), 1U);
  ASSERT_EQ(where.disjunctions[1].size(), 2U);
}

TEST(SqlParserTest, RejectsDanglingAndOrAndRepeatedConnectors) {
  SqlParserImpl parser;
  for (const auto *sql : {"SELECT * FROM users WHERE id = 1 AND",
                          "SELECT * FROM users WHERE id = 1 OR",
                          "SELECT * FROM users WHERE id = 1 AND OR id = 2"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

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
       {"SELECT * FROM users LIMIT",
        "SELECT * FROM users LIMIT -1",
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
