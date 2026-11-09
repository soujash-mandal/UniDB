#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserTypeDeclarationTest, ParsesIntegerTypeNamesAndAliases) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("CREATE TABLE numbers (a SMALLINT, b INT2, c INTEGER, "
                   "d INT4, e BIGINT, f INT8)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &columns =
      std::get<CreateTableStatement>(std::get<SqlStatement>(result)).columns;
  ASSERT_EQ(columns.size(), 6U);
  EXPECT_EQ(columns[0].declaredType->id, SqlTypeId::SmallInt);
  EXPECT_EQ(columns[1].declaredType->id, SqlTypeId::SmallInt);
  EXPECT_EQ(columns[2].declaredType->id, SqlTypeId::Integer);
  EXPECT_EQ(columns[3].declaredType->id, SqlTypeId::Integer);
  EXPECT_EQ(columns[4].declaredType->id, SqlTypeId::BigInt);
  EXPECT_EQ(columns[5].declaredType->id, SqlTypeId::BigInt);
}

TEST(SqlParserTypeDeclarationTest, ParsesCoreNamesAliasesAndModifiers) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "CREATE TABLE typed (amount NUMERIC(12, 3), label CHARACTER VARYING(40), "
      "fixed CHAR(4), score DOUBLE PRECISION, active BOOLEAN, alias BOOL, "
      "created TIMESTAMP WITH TIME ZONE, precise TIMESTAMP(3) WITH TIME ZONE)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &columns =
      std::get<CreateTableStatement>(std::get<SqlStatement>(result)).columns;
  ASSERT_EQ(columns.size(), 8U);
  EXPECT_EQ(columns[0].declaredType->id, SqlTypeId::Numeric);
  EXPECT_EQ(columns[0].declaredType->modifier.precision, 12U);
  EXPECT_EQ(columns[0].declaredType->modifier.scale, 3);
  EXPECT_EQ(columns[1].declaredType->id, SqlTypeId::Varchar);
  EXPECT_EQ(columns[1].declaredType->modifier.length, 40U);
  EXPECT_EQ(columns[2].declaredType->id, SqlTypeId::Char);
  EXPECT_EQ(columns[3].declaredType->id, SqlTypeId::DoublePrecision);
  EXPECT_EQ(columns[4].declaredType->id, SqlTypeId::Boolean);
  EXPECT_EQ(columns[5].declaredType->id, SqlTypeId::Boolean);
  EXPECT_EQ(columns[6].declaredType->id, SqlTypeId::TimestampWithTimeZone);
  EXPECT_EQ(columns[7].declaredType->id, SqlTypeId::TimestampWithTimeZone);
  EXPECT_EQ(columns[7].declaredType->modifier.temporalPrecision, 3U);
}

TEST(SqlParserTypeDeclarationTest, PreservesTransitionalIntAndTextTags) {
  SqlParserImpl parser;
  const auto result = parser.parse("CREATE TABLE legacy (id INT, name TEXT)");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &columns =
      std::get<CreateTableStatement>(std::get<SqlStatement>(result)).columns;
  ASSERT_EQ(columns.size(), 2U);
  EXPECT_EQ(columns[0].type, SqlType::Int);
  EXPECT_EQ(columns[0].declaredType->id, SqlTypeId::Integer);
  EXPECT_EQ(columns[1].type, SqlType::Text);
  EXPECT_EQ(columns[1].declaredType->id, SqlTypeId::Text);
}

TEST(SqlParserTypeDeclarationTest, RejectsInvalidTypeModifiers) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "CREATE TABLE t (a SMALLINT(2))",
           "CREATE TABLE t (a VARCHAR(0))",
           "CREATE TABLE t (a NUMERIC(10, 1001))",
           "CREATE TABLE t (a TIMESTAMP(7))",
           "CREATE TABLE t (a NUMERIC(10, 2, 1))",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTypeDeclarationTest, RejectsUnknownTypeNames) {
  SqlParserImpl parser;
  const auto result = parser.parse("CREATE TABLE t (a MONEY)");
  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

} // namespace
} // namespace unidb::sql
