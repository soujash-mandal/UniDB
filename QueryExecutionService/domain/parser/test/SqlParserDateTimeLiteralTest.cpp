#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserDateTimeLiteralTest, PreservesAllSixTemporalLiteralTypes) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO events (d, t, tz, ts, tstz, duration) VALUES "
      "(DATE '2026-11-11', TIME '12:34:56', "
      "TIME WITH TIME ZONE '12:34:56+05:30', "
      "TIMESTAMP '2026-11-11 12:34:56', "
      "TIMESTAMP WITH TIME ZONE '2026-11-11 12:34:56+05:30', "
      "INTERVAL '1 day 2 hours')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &statement = std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(statement.rows.size(), 1U);
  ASSERT_EQ(statement.typedLiterals.size(), 1U);
  ASSERT_EQ(statement.typedLiterals[0].size(), 6U);

  const auto &typed = statement.typedLiterals[0];
  for (const auto &literal : typed) {
    ASSERT_TRUE(literal.has_value());
  }
  EXPECT_EQ(typed[0]->declaredType.id, SqlTypeId::Date);
  EXPECT_EQ(typed[1]->declaredType.id, SqlTypeId::Time);
  EXPECT_EQ(typed[2]->declaredType.id, SqlTypeId::TimeWithTimeZone);
  EXPECT_EQ(typed[3]->declaredType.id, SqlTypeId::Timestamp);
  EXPECT_EQ(typed[4]->declaredType.id, SqlTypeId::TimestampWithTimeZone);
  EXPECT_EQ(typed[5]->declaredType.id, SqlTypeId::Interval);
  EXPECT_EQ(typed[0]->literal, "2026-11-11");
  EXPECT_EQ(typed[4]->literal, "2026-11-11 12:34:56+05:30");
}

TEST(SqlParserDateTimeLiteralTest, KeepsLocalAndInstantTimestampsDistinct) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT * FROM events WHERE local_time = "
      "TIMESTAMP '2026-11-11 12:34:56'");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select = std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
  const auto &predicate =
      std::get<WhereExpression>(select.where).predicate;
  ASSERT_TRUE(predicate.typedLiteral.has_value());
  EXPECT_EQ(predicate.typedLiteral->declaredType.id, SqlTypeId::Timestamp);
  EXPECT_EQ(predicate.typedLiteral->literal, "2026-11-11 12:34:56");
}

TEST(SqlParserDateTimeLiteralTest, RetainsTypedLiteralMetadataAlongsideRuntimeValue) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "INSERT INTO events (created_at, label) VALUES "
      "(TIMESTAMPTZ '2026-11-11 12:34:56+00:00', 'launch')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &statement = std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(statement.typedLiterals[0].size(), 2U);
  EXPECT_TRUE(statement.typedLiterals[0][0].has_value());
  EXPECT_FALSE(statement.typedLiterals[0][1].has_value());
  EXPECT_EQ(statement.rows[0][0], SqlValue{std::string("2026-11-11 12:34:56+00:00")});
}

TEST(SqlParserDateTimeLiteralTest, RejectsTypedLiteralWithoutQuotedValue) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "INSERT INTO events (created_at) VALUES (DATE 2026)",
           "INSERT INTO events (created_at) VALUES (TIMESTAMP)",
           "SELECT * FROM events WHERE created_at = INTERVAL 5",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
