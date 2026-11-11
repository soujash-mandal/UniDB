#include "../../domain/types/SqlTimeArithmetic.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace domain = QueryExecutionService::domain;

namespace {

constexpr std::int64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::int64_t kMicrosecondsPerMinute = 60 * kMicrosecondsPerSecond;
constexpr std::int64_t kMicrosecondsPerHour = 60 * kMicrosecondsPerMinute;
constexpr std::int64_t kMicrosecondsPerDay = 24 * kMicrosecondsPerHour;

} // namespace

TEST(SqlTimeArithmeticTest, AddsIntervalAndWrapsAcrossMidnight) {
  const domain::SqlTime time{23 * kMicrosecondsPerHour};

  EXPECT_EQ(domain::addSqlTimeInterval(
                time, domain::SqlInterval{0, 0, 2 * kMicrosecondsPerHour}),
            (domain::SqlTime{kMicrosecondsPerHour}));
}

TEST(SqlTimeArithmeticTest, SupportsCommutativeIntervalAddition) {
  const domain::SqlTime time{3 * kMicrosecondsPerHour};
  const domain::SqlInterval interval{0, 0, 30 * kMicrosecondsPerMinute};

  EXPECT_EQ(domain::addSqlTimeInterval(interval, time),
            domain::addSqlTimeInterval(time, interval));
}

TEST(SqlTimeArithmeticTest, SubtractsIntervalAndWrapsBeforeMidnight) {
  const domain::SqlTime time{30 * kMicrosecondsPerMinute};

  EXPECT_EQ(domain::subtractSqlTimeInterval(
                time, domain::SqlInterval{0, 0, kMicrosecondsPerHour}),
            (domain::SqlTime{23 * kMicrosecondsPerHour +
                             30 * kMicrosecondsPerMinute}));
}

TEST(SqlTimeArithmeticTest, IgnoresIntervalMonthsAndDays) {
  const domain::SqlTime time{4 * kMicrosecondsPerHour};

  EXPECT_EQ(domain::addSqlTimeInterval(time, domain::SqlInterval{12, 7, 0}),
            time);
  EXPECT_EQ(domain::subtractSqlTimeInterval(time,
                                            domain::SqlInterval{-3, -9, 0}),
            time);
}

TEST(SqlTimeArithmeticTest, PreservesSubsecondPrecision) {
  const domain::SqlTime time{123};
  const domain::SqlInterval interval{0, 0, 456};

  EXPECT_EQ(domain::addSqlTimeInterval(time, interval),
            (domain::SqlTime{579}));
  EXPECT_EQ(domain::subtractSqlTimeInterval(time, interval),
            (domain::SqlTime{kMicrosecondsPerDay - 333}));
}

TEST(SqlTimeArithmeticTest, HandlesZeroInterval) {
  const domain::SqlTime time{12 * kMicrosecondsPerHour + 34};

  EXPECT_EQ(domain::addSqlTimeInterval(time, domain::SqlInterval{}), time);
  EXPECT_EQ(domain::subtractSqlTimeInterval(time, domain::SqlInterval{}),
            time);
}

TEST(SqlTimeArithmeticTest, IgnoresLargePositiveAndNegativeIntervalDays) {
  const domain::SqlTime time{5 * kMicrosecondsPerHour};

  EXPECT_EQ(domain::addSqlTimeInterval(
                time, domain::SqlInterval{0, 0, 3 * kMicrosecondsPerDay}),
            time);
  EXPECT_EQ(domain::addSqlTimeInterval(
                time, domain::SqlInterval{0, 0, -3 * kMicrosecondsPerDay}),
            time);
}

TEST(SqlTimeArithmeticTest, HandlesExtremeIntervalMicrosecondsWithoutOverflow) {
  const domain::SqlTime midnight{0};
  const auto maxRemainder =
      std::numeric_limits<std::int64_t>::max() % kMicrosecondsPerDay;
  const auto minRemainder =
      std::numeric_limits<std::int64_t>::min() % kMicrosecondsPerDay;
  const auto positiveExpected =
      maxRemainder < 0 ? maxRemainder + kMicrosecondsPerDay : maxRemainder;
  const auto negativeExpected =
      minRemainder < 0 ? minRemainder + kMicrosecondsPerDay : minRemainder;

  EXPECT_EQ(domain::addSqlTimeInterval(
                midnight, domain::SqlInterval{0, 0,
                                              std::numeric_limits<std::int64_t>::max()}),
            (domain::SqlTime{positiveExpected}));
  EXPECT_EQ(domain::addSqlTimeInterval(
                midnight, domain::SqlInterval{0, 0,
                                              std::numeric_limits<std::int64_t>::min()}),
            (domain::SqlTime{negativeExpected}));
}

TEST(SqlTimeArithmeticTest, TimeDifferenceReturnsSignedTimeOnlyInterval) {
  const domain::SqlTime later{8 * kMicrosecondsPerHour + 15};
  const domain::SqlTime earlier{3 * kMicrosecondsPerHour + 5};

  EXPECT_EQ(domain::differenceSqlTimes(later, earlier),
            (domain::SqlInterval{0, 0, 5 * kMicrosecondsPerHour + 10}));
  EXPECT_EQ(domain::differenceSqlTimes(earlier, later),
            (domain::SqlInterval{0, 0, -(5 * kMicrosecondsPerHour + 10)}));
}

TEST(SqlTimeArithmeticTest, TimeDifferenceAllowsTwentyFourHourEndpoint) {
  EXPECT_EQ(domain::differenceSqlTimes(domain::SqlTime{kMicrosecondsPerDay},
                                       domain::SqlTime{0}),
            (domain::SqlInterval{0, 0, kMicrosecondsPerDay}));
}

TEST(SqlTimeArithmeticTest, RejectsInvalidTimeValues) {
  EXPECT_THROW(domain::addSqlTimeInterval(
                   domain::SqlTime{-1}, domain::SqlInterval{}),
               std::out_of_range);
  EXPECT_THROW(domain::subtractSqlTimeInterval(
                   domain::SqlTime{kMicrosecondsPerDay + 1},
                   domain::SqlInterval{}),
               std::out_of_range);
  EXPECT_THROW(domain::differenceSqlTimes(domain::SqlTime{-1},
                                          domain::SqlTime{0}),
               std::out_of_range);
}
