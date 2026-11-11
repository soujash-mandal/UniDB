#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserDateTimeLiteralTest, PreservesAllSixTemporalLiteralTypes) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO events (d, t, tz, ts, tstz, duration) VALUES "
                   "(DATE '2026-11-11', TIME '12:34:56', "
                   "TIME WITH TIME ZONE '12:34:56+05:30', "
                   "TIMESTAMP '2026-11-11 12:34:56', "
                   "TIMESTAMP WITH TIME ZONE '2026-11-11 12:34:56+05:30', "
                   "INTERVAL '1 day 2 hours')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &statement =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
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
  const auto result = parser.parse("SELECT * FROM events WHERE local_time = "
                                   "TIMESTAMP '2026-11-11 12:34:56'");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
  const auto &predicate = std::get<WhereExpression>(select.where).predicate;
  ASSERT_TRUE(predicate.typedLiteral.has_value());
  EXPECT_EQ(predicate.typedLiteral->declaredType.id, SqlTypeId::Timestamp);
  EXPECT_EQ(predicate.typedLiteral->literal, "2026-11-11 12:34:56");
}

TEST(SqlParserDateTimeLiteralTest,
     RetainsTypedLiteralMetadataAlongsideRuntimeValue) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO events (created_at, label) VALUES "
                   "(TIMESTAMPTZ '2026-11-11 12:34:56+00:00', 'launch')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &statement =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(statement.typedLiterals[0].size(), 2U);
  EXPECT_TRUE(statement.typedLiterals[0][0].has_value());
  EXPECT_FALSE(statement.typedLiterals[0][1].has_value());
  EXPECT_EQ(statement.rows[0][0],
            SqlValue{std::string("2026-11-11 12:34:56+00:00")});
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

TEST(SqlParserDateTimeLiteralTest, ParsesAndValidatesCalendarAndClockValues) {
  SqlParserImpl parser;
  const auto valid =
      parser.parse("INSERT INTO events (d, t) VALUES "
                   "(DATE '2024-02-29', TIME(3) '12:34:56.789')");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(valid));
  const auto &insert = std::get<InsertStatement>(std::get<SqlStatement>(valid));
  ASSERT_EQ(insert.typedLiterals[0].size(), 2U);
  EXPECT_TRUE(std::holds_alternative<QueryExecutionService::domain::SqlDate>(
      insert.typedLiterals[0][0]->value));
  const auto &time = std::get<QueryExecutionService::domain::SqlTime>(
      insert.typedLiterals[0][1]->value);
  const auto expectedMicroseconds =
      static_cast<std::int64_t>(12 * 3600 + 34 * 60 + 56) * 1'000'000 +
      789'000;
  EXPECT_EQ(time.microsecondsSinceMidnight, expectedMicroseconds);
}

TEST(SqlParserDateTimeLiteralTest, NormalizesExplicitOffsetsToUtcInstant) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO events (instant) VALUES "
                                   "(TIMESTAMPTZ '2026-11-11 12:34:56+05:30')");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  const auto &instant =
      std::get<QueryExecutionService::domain::SqlTimestampWithTimeZone>(
          insert.typedLiterals[0][0]->value);
  const auto date = QueryExecutionService::domain::makeSqlDate(2026, 11, 11);
  const auto expected =
      static_cast<std::int64_t>(date.daysSinceEpoch) * 86'400'000'000LL +
      (12 * 3600 + 34 * 60 + 56 - 5 * 3600 - 30 * 60) * 1'000'000LL;
  EXPECT_EQ(instant.microsecondsSinceEpoch, expected);
}

TEST(SqlParserDateTimeLiteralTest, ParsesIntervalCalendarAndClockFields) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO events (duration) VALUES (INTERVAL '1 year 2 "
                   "months 3 days 4 hours 5 minutes 6.5 seconds')");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  const auto &interval = std::get<QueryExecutionService::domain::SqlInterval>(
      insert.typedLiterals[0][0]->value);
  EXPECT_EQ(interval.months, 14);
  EXPECT_EQ(interval.days, 3);
  EXPECT_EQ(interval.microseconds,
            (4 * 3600 + 5 * 60) * 1'000'000LL + 6'500'000LL);
}

TEST(SqlParserDateTimeLiteralTest, RejectsInvalidTemporalValues) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "INSERT INTO events (d) VALUES (DATE '2025-02-29')",
           "INSERT INTO events (d) VALUES (DATE '2026-13-01')",
           "INSERT INTO events (t) VALUES (TIME '25:00:00')",
           "INSERT INTO events (t) VALUES (TIME '12:61:00')",
           "INSERT INTO events (tz) VALUES (TIMETZ '12:00:00')",
           "INSERT INTO events (tz) VALUES (TIMETZ '12:00:00+16:00')",
           "INSERT INTO events (ts) VALUES (TIMESTAMP '2026-11-11 "
           "12:00:00+00:00')",
           "INSERT INTO events (ts) VALUES (TIMESTAMP '2026-02-30 12:00:00')",
           "INSERT INTO events (tstz) VALUES (TIMESTAMPTZ '2026-11-11 "
           "12:00:00')",
           "INSERT INTO events (duration) VALUES (INTERVAL '3 fortnights')",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

TEST(SqlParserDateTimeLiteralTest, RoundsToDeclaredTemporalPrecision) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO events (t) VALUES (TIME(2) '12:34:56.789')");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  const auto &time = std::get<QueryExecutionService::domain::SqlTime>(
      insert.typedLiterals[0][0]->value);
  const auto expectedMicroseconds =
      static_cast<std::int64_t>(12 * 3600 + 34 * 60 + 56) * 1'000'000 +
      790'000;
  EXPECT_EQ(time.microsecondsSinceMidnight, expectedMicroseconds);
}

} // namespace
} // namespace unidb::sql
