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
  ASSERT_EQ(insert.rows.size(), 1U);
  ASSERT_EQ(insert.rows[0].size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(insert.rows[0][0]), -42);
  EXPECT_EQ(std::get<std::string>(insert.rows[0][1]), "O'Reilly");
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
  EXPECT_EQ(std::get<WhereExpression>(select.where).predicate.comparison,
            ComparisonOperator::GreaterThan);
}

TEST(SqlParserTest, RejectsMismatchedInsertColumnAndValueCounts) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO users (id, name) VALUES (1)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, ParsesBooleanInsertLiteralsCaseInsensitively) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO users (enabled) VALUES (TRUE), (false), (TrUe)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 3U);
  EXPECT_TRUE(std::get<bool>(insert.rows[0][0]));
  EXPECT_FALSE(std::get<bool>(insert.rows[1][0]));
  EXPECT_TRUE(std::get<bool>(insert.rows[2][0]));
}

TEST(SqlParserTest, ParsesMultipleInsertValueRows) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO users (id, name) VALUES (1, 'Ada'), (2, 'Grace')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 2U);
  ASSERT_EQ(insert.rows[0].size(), 2U);
  ASSERT_EQ(insert.rows[1].size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(insert.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::string>(insert.rows[0][1]), "Ada");
  EXPECT_EQ(std::get<std::int64_t>(insert.rows[1][0]), 2);
  EXPECT_EQ(std::get<std::string>(insert.rows[1][1]), "Grace");
}

TEST(SqlParserTest, RejectsMalformedMultiRowInsertValues) {
  SqlParserImpl parser;
  for (const auto *sql :
       {"INSERT INTO users (id, name) VALUES (1, 'Ada'),",
        "INSERT INTO users (id, name) VALUES (1, 'Ada'), (2)",
        "INSERT INTO users (id, name) VALUES (1, 'Ada'), ()",
        "INSERT INTO users (id, name) VALUES (1, 'Ada'), (2, 'Grace'"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, ParsesDecimalLiteralsAsExactNumericValues) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO measurements (a, b, c, d) VALUES "
                   "(1.25, -2.5, 1e3, 1.25e-2)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 1U);
  ASSERT_EQ(insert.rows[0].size(), 4U);
  EXPECT_EQ(std::get<SqlNumericValue>(insert.rows[0][0]),
            (SqlNumericValue{false, "125", 2}));
  EXPECT_EQ(std::get<SqlNumericValue>(insert.rows[0][1]),
            (SqlNumericValue{true, "25", 1}));
  EXPECT_EQ(std::get<SqlNumericValue>(insert.rows[0][2]),
            (SqlNumericValue{false, "1", -3}));
  EXPECT_EQ(std::get<SqlNumericValue>(insert.rows[0][3]),
            (SqlNumericValue{false, "125", 4}));
}

TEST(SqlParserTest, KeepsIntegerLiteralsAsIntegersBesideFloatingPointValues) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO measurements (whole, fractional) VALUES (7, 7.0)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 1U);
  EXPECT_TRUE(std::holds_alternative<std::int64_t>(insert.rows[0][0]));
  EXPECT_TRUE(std::holds_alternative<SqlNumericValue>(insert.rows[0][1]));
}

TEST(SqlParserTest, RejectsMalformedFloatingPointExponents) {
  SqlParserImpl parser;
  for (const auto *sql : {"INSERT INTO measurements (value) VALUES (1e)",
                          "INSERT INTO measurements (value) VALUES (1.0e+)"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, ParsesLeadingAndTrailingDecimalPointForms) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO measurements (leading, trailing) VALUES (.5, 1.)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 1U);
  EXPECT_EQ(std::get<SqlNumericValue>(insert.rows[0][0]),
            (SqlNumericValue{false, "5", 1}));
  EXPECT_EQ(std::get<SqlNumericValue>(insert.rows[0][1]),
            (SqlNumericValue{false, "1", 0}));
}

} // namespace
} // namespace unidb::sql


TEST(SqlParserTest, ParsesNullInsertValuesCaseInsensitively) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO users (id, name) VALUES (1, NULL), (2, null)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.rows.size(), 2U);
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(insert.rows[0][1]));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(insert.rows[1][1]));
}

TEST(SqlParserTest, ParsesNullableAndNotNullColumnConstraints) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("CREATE TABLE users (id INT NOT NULL, name TEXT NULL, "
                   "nickname TEXT)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &create =
      std::get<CreateTableStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(create.columns.size(), 3U);
  EXPECT_FALSE(create.columns[0].nullable);
  EXPECT_TRUE(create.columns[1].nullable);
  EXPECT_TRUE(create.columns[2].nullable);
}

TEST(SqlParserTest, RejectsMalformedNotNullConstraint) {
  SqlParserImpl parser;
  const auto result = parser.parse("CREATE TABLE users (id INT NOT)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}
