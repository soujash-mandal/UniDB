#include "../../domain/types/SqlTimestampArithmetic.h"

#include "../../domain/types/SqlDateTimeSemantics.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace domain = QueryExecutionService::domain;

namespace {

constexpr std::int64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::int64_t kMicrosecondsPerHour = 3'600 * kMicrosecondsPerSecond;
constexpr std::int64_t kMicrosecondsPerDay = 24 * kMicrosecondsPerHour;

domain::SqlTimestamp timestamp(std::int32_t year, std::uint8_t month,
                               std::uint8_t day, std::int64_t time) {
  return domain::SqlTimestamp{domain::makeSqlDate(year, month, day),
                              domain::SqlTime{time}};
}

} // namespace

TEST(SqlTimestampArithmeticTest, AddsCalendarMonthsAndClampsAtMonthEnd) {
  const auto january31 = timestamp(2024, 1, 31, 12 * kMicrosecondsPerHour);

  EXPECT_EQ(
      domain::addSqlTimestampInterval(january31, domain::SqlInterval{1, 0, 0}),
      timestamp(2024, 2, 29, 12 * kMicrosecondsPerHour));
  EXPECT_EQ(domain::addSqlTimestampInterval(timestamp(2023, 1, 31, 0),
                                            domain::SqlInterval{1, 0, 0}),
            timestamp(2023, 2, 28, 0));
  EXPECT_EQ(domain::addSqlTimestampInterval(timestamp(2024, 3, 31, 0),
                                            domain::SqlInterval{-1, 0, 0}),
            timestamp(2024, 2, 29, 0));
}

TEST(SqlTimestampArithmeticTest, AddsIntervalInCalendarComponentOrder) {
  const auto start = timestamp(2024, 1, 31, 23 * kMicrosecondsPerHour);

  EXPECT_EQ(domain::addSqlTimestampInterval(
                start, domain::SqlInterval{1, 1, kMicrosecondsPerHour}),
            timestamp(2024, 3, 2, 0));
}

TEST(SqlTimestampArithmeticTest, PreservesLocalTimeWhenAddingCalendarDays) {
  const auto start = timestamp(2024, 2, 28, 7 * kMicrosecondsPerHour + 123);

  EXPECT_EQ(
      domain::addSqlTimestampInterval(start, domain::SqlInterval{0, 1, 0}),
      timestamp(2024, 2, 29, 7 * kMicrosecondsPerHour + 123));
}

TEST(SqlTimestampArithmeticTest, MicrosecondsCarryAndBorrowAcrossMidnight) {
  const auto start = timestamp(2024, 3, 1, 30 * kMicrosecondsPerSecond);

  EXPECT_EQ(domain::addSqlTimestampInterval(
                start, domain::SqlInterval{0, 0, -60 * kMicrosecondsPerSecond}),
            timestamp(2024, 2, 29,
                      23 * kMicrosecondsPerHour + 59 * kMicrosecondsPerSecond +
                          30 * kMicrosecondsPerSecond));
  EXPECT_EQ(
      domain::subtractSqlTimestampInterval(
          timestamp(2024, 3, 1, 0),
          domain::SqlInterval{0, 0, kMicrosecondsPerSecond}),
      timestamp(2024, 2, 29, kMicrosecondsPerDay - kMicrosecondsPerSecond));
}

TEST(SqlTimestampArithmeticTest, SupportsCommutativeIntervalAddition) {
  const auto value = timestamp(2024, 4, 10, 123);

  EXPECT_EQ(
      domain::addSqlTimestampInterval(domain::SqlInterval{0, 2, 5}, value),
      domain::addSqlTimestampInterval(value, domain::SqlInterval{0, 2, 5}));
}

TEST(SqlTimestampArithmeticTest, TimestampDifferenceReturnsTimeOnlyInterval) {
  const auto left = timestamp(2024, 3, 1, 10 * kMicrosecondsPerHour + 25);
  const auto right = timestamp(2024, 2, 28, 9 * kMicrosecondsPerHour + 10);

  EXPECT_EQ(domain::differenceSqlTimestamps(left, right),
            (domain::SqlInterval{0, 0, 49 * kMicrosecondsPerHour + 15}));
  EXPECT_EQ(domain::differenceSqlTimestamps(right, left),
            (domain::SqlInterval{0, 0, -(49 * kMicrosecondsPerHour + 15)}));
}

TEST(SqlTimestampArithmeticTest, HandlesLeapDayAndYearBoundaries) {
  EXPECT_EQ(domain::addSqlTimestampInterval(timestamp(2023, 12, 31, 0),
                                            domain::SqlInterval{0, 1, 0}),
            timestamp(2024, 1, 1, 0));
  EXPECT_EQ(domain::addSqlTimestampInterval(timestamp(2024, 2, 29, 0),
                                            domain::SqlInterval{12, 0, 0}),
            timestamp(2025, 2, 28, 0));
}

TEST(SqlTimestampArithmeticTest, RejectsOutOfRangeResultsAndOverflow) {
  const auto maximum = timestamp(294276, 12, 31, kMicrosecondsPerDay - 1);

  EXPECT_THROW(
      domain::addSqlTimestampInterval(maximum, domain::SqlInterval{0, 0, 1}),
      std::out_of_range);
  EXPECT_THROW(
      domain::subtractSqlTimestampInterval(
          timestamp(2000, 1, 1, 0),
          domain::SqlInterval{0, 0, std::numeric_limits<std::int64_t>::min()}),
      std::out_of_range);
  EXPECT_THROW(
      domain::addSqlTimestampInterval(
          timestamp(2000, 1, 1, 0),
          domain::SqlInterval{std::numeric_limits<std::int32_t>::max(), 0, 0}),
      std::out_of_range);
}

TEST(SqlTimestampArithmeticTest, RejectsInvalidTimeComponents) {
  EXPECT_THROW(domain::addSqlTimestampInterval(
                   domain::SqlTimestamp{domain::makeSqlDate(2024, 1, 1),
                                        domain::SqlTime{-1}},
                   domain::SqlInterval{}),
               std::out_of_range);
}
