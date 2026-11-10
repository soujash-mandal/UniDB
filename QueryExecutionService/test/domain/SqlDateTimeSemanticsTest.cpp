#include "../../domain/types/SqlDateTimeSemantics.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

namespace domain = QueryExecutionService::domain;

TEST(SqlDateTimeSemanticsTest, ConvertsGregorianDatesRelativeToPostgresEpoch) {
  EXPECT_EQ(domain::makeSqlDate(2000, 1, 1), domain::SqlDate{0});
  EXPECT_EQ(domain::makeSqlDate(1999, 12, 31), domain::SqlDate{-1});
  EXPECT_EQ(domain::makeSqlDate(2000, 1, 2), domain::SqlDate{1});
}

TEST(SqlDateTimeSemanticsTest, AcceptsLeapDaysOnlyInLeapYears) {
  EXPECT_NO_THROW(domain::makeSqlDate(2000, 2, 29));
  EXPECT_NO_THROW(domain::makeSqlDate(2024, 2, 29));
  EXPECT_THROW(domain::makeSqlDate(1900, 2, 29), std::invalid_argument);
  EXPECT_THROW(domain::makeSqlDate(2023, 2, 29), std::invalid_argument);
}

TEST(SqlDateTimeSemanticsTest, ConvertsBcDatesAcrossTheEraBoundary) {
  const auto lastBcDay = domain::makeSqlDate(-1, 12, 31);
  const auto firstAdDay = domain::makeSqlDate(1, 1, 1);

  EXPECT_EQ(firstAdDay.daysSincePostgresEpoch -
                lastBcDay.daysSincePostgresEpoch,
            1);
}

TEST(SqlDateTimeSemanticsTest, AppliesLeapYearRulesToBcYearNumbering) {
  EXPECT_NO_THROW(domain::makeSqlDate(-1, 2, 29));
  EXPECT_THROW(domain::makeSqlDate(-4, 2, 29), std::invalid_argument);
}

TEST(SqlDateTimeSemanticsTest, RejectsInvalidCalendarComponents) {
  EXPECT_THROW(domain::makeSqlDate(2000, 0, 1), std::invalid_argument);
  EXPECT_THROW(domain::makeSqlDate(2000, 13, 1), std::invalid_argument);
  EXPECT_THROW(domain::makeSqlDate(2000, 4, 31), std::invalid_argument);
  EXPECT_THROW(domain::makeSqlDate(0, 1, 1), std::invalid_argument);
}

TEST(SqlDateTimeSemanticsTest, ValidatesTimeIncludingPostgresEndOfDayValue) {
  constexpr std::int64_t day = 86'400LL * 1'000'000LL;
  EXPECT_NO_THROW(domain::validateSqlTime(domain::SqlTime{0}));
  EXPECT_NO_THROW(domain::validateSqlTime(domain::SqlTime{day}));
  EXPECT_THROW(domain::validateSqlTime(domain::SqlTime{-1}), std::out_of_range);
  EXPECT_THROW(domain::validateSqlTime(domain::SqlTime{day + 1}),
               std::out_of_range);
}

TEST(SqlDateTimeSemanticsTest, ValidatesTimeZoneOffsetBoundaries) {
  constexpr std::int32_t maximum = 15 * 3600 + 59 * 60 + 59;
  EXPECT_NO_THROW(domain::validateSqlTimeZoneOffset(maximum));
  EXPECT_NO_THROW(domain::validateSqlTimeZoneOffset(-maximum));
  EXPECT_THROW(domain::validateSqlTimeZoneOffset(maximum + 1),
               std::out_of_range);
  EXPECT_THROW(domain::validateSqlTimeZoneOffset(-maximum - 1),
               std::out_of_range);
}

TEST(SqlDateTimeSemanticsTest, TimestampReusesTimeValidation) {
  EXPECT_NO_THROW(domain::validateSqlTimestamp(
      domain::SqlTimestamp{domain::SqlDate{0}, domain::SqlTime{0}}));
  EXPECT_THROW(domain::validateSqlTimestamp(domain::SqlTimestamp{
                   domain::SqlDate{0}, domain::SqlTime{-1}}),
               std::out_of_range);
}

TEST(SqlDateTimeSemanticsTest, RoundsFractionalSecondsForEveryPrecision) {
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 0), 0);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 1), 100000);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 2), 120000);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 3), 123000);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 4), 123500);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 5), 123460);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(123456, 6), 123456);
}

TEST(SqlDateTimeSemanticsTest, RoundingCarriesIntoNextSecond) {
  EXPECT_EQ(domain::roundSqlFractionalSeconds(999999, 0), 1'000'000);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(999999, 3), 1'000'000);
  EXPECT_EQ(domain::roundSqlFractionalSeconds(999499, 3), 999000);
}

TEST(SqlDateTimeSemanticsTest, RejectsInvalidFractionalPrecisionAndValue) {
  EXPECT_THROW(domain::roundSqlFractionalSeconds(0, 7), std::invalid_argument);
  EXPECT_THROW(domain::roundSqlFractionalSeconds(-1, 3), std::out_of_range);
  EXPECT_THROW(domain::roundSqlFractionalSeconds(1'000'000, 3),
               std::out_of_range);
}
