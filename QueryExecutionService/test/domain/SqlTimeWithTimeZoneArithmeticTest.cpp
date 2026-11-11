#include "../../domain/types/SqlTimeWithTimeZoneArithmetic.h"

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

TEST(SqlTimeWithTimeZoneArithmeticTest, AddsIntervalAndWrapsAcrossMidnight) {
  const domain::SqlTimeWithTimeZone time{
      domain::SqlTime{23 * kMicrosecondsPerHour}, 5 * 60 * 60};

  EXPECT_EQ(domain::addSqlTimeWithTimeZoneInterval(
                time, domain::SqlInterval{0, 0, 2 * kMicrosecondsPerHour}),
            (domain::SqlTimeWithTimeZone{domain::SqlTime{kMicrosecondsPerHour},
                                         5 * 60 * 60}));
}

TEST(SqlTimeWithTimeZoneArithmeticTest,
     SubtractsIntervalAndWrapsBeforeMidnight) {
  const domain::SqlTimeWithTimeZone time{
      domain::SqlTime{30 * kMicrosecondsPerMinute}, -4 * 60 * 60};

  EXPECT_EQ(domain::subtractSqlTimeWithTimeZoneInterval(
                time, domain::SqlInterval{0, 0, kMicrosecondsPerHour}),
            (domain::SqlTimeWithTimeZone{
                domain::SqlTime{23 * kMicrosecondsPerHour +
                                30 * kMicrosecondsPerMinute},
                -4 * 60 * 60}));
}

TEST(SqlTimeWithTimeZoneArithmeticTest,
     IgnoresIntervalMonthsAndDaysAndPreservesOffset) {
  const domain::SqlTimeWithTimeZone time{
      domain::SqlTime{4 * kMicrosecondsPerHour + 17}, 5 * 60 * 30};

  EXPECT_EQ(
      domain::addSqlTimeWithTimeZoneInterval(
          time, domain::SqlInterval{12, 7, 0}),
      time);
  EXPECT_EQ(domain::subtractSqlTimeWithTimeZoneInterval(
                time, domain::SqlInterval{-3, -9, 0}),
            time);
}

TEST(SqlTimeWithTimeZoneArithmeticTest, PreservesSubsecondPrecision) {
  const domain::SqlTimeWithTimeZone time{domain::SqlTime{123}, 19'800};

  EXPECT_EQ(domain::addSqlTimeWithTimeZoneInterval(
                time, domain::SqlInterval{0, 0, 456}),
            (domain::SqlTimeWithTimeZone{domain::SqlTime{579}, 19'800}));
  EXPECT_EQ(domain::subtractSqlTimeWithTimeZoneInterval(
                time, domain::SqlInterval{0, 0, 456}),
            (domain::SqlTimeWithTimeZone{
                domain::SqlTime{kMicrosecondsPerDay - 333}, 19'800}));
}

TEST(SqlTimeWithTimeZoneArithmeticTest, WrapsTwentyFourHourEndpoint) {
  const domain::SqlTimeWithTimeZone endOfDay{
      domain::SqlTime{kMicrosecondsPerDay}, 0};

  EXPECT_EQ(domain::addSqlTimeWithTimeZoneInterval(
                endOfDay, domain::SqlInterval{}),
            (domain::SqlTimeWithTimeZone{domain::SqlTime{0}, 0}));
}

TEST(SqlTimeWithTimeZoneArithmeticTest,
     HandlesExtremeIntervalMicrosecondsWithoutOverflow) {
  const domain::SqlTimeWithTimeZone midnight{domain::SqlTime{0}, 19'800};
  const auto maxRemainder =
      std::numeric_limits<std::int64_t>::max() % kMicrosecondsPerDay;
  const auto minRemainder =
      std::numeric_limits<std::int64_t>::min() % kMicrosecondsPerDay;
  const auto positiveExpected =
      maxRemainder < 0 ? maxRemainder + kMicrosecondsPerDay : maxRemainder;
  const auto negativeExpected =
      minRemainder < 0 ? minRemainder + kMicrosecondsPerDay : minRemainder;
  const auto subtractMinimumExpected =
      (kMicrosecondsPerDay - negativeExpected) % kMicrosecondsPerDay;

  EXPECT_EQ(domain::addSqlTimeWithTimeZoneInterval(
                midnight,
                domain::SqlInterval{
                    0, 0, std::numeric_limits<std::int64_t>::max()}),
            (domain::SqlTimeWithTimeZone{
                domain::SqlTime{positiveExpected}, 19'800}));
  EXPECT_EQ(domain::subtractSqlTimeWithTimeZoneInterval(
                midnight,
                domain::SqlInterval{
                    0, 0, std::numeric_limits<std::int64_t>::min()}),
            (domain::SqlTimeWithTimeZone{
                domain::SqlTime{subtractMinimumExpected}, 19'800}));
}

TEST(SqlTimeWithTimeZoneArithmeticTest, RejectsInvalidTimeAndOffsetValues) {
  EXPECT_THROW(domain::addSqlTimeWithTimeZoneInterval(
                   domain::SqlTimeWithTimeZone{domain::SqlTime{-1}, 0},
                   domain::SqlInterval{}),
               std::out_of_range);
  EXPECT_THROW(domain::subtractSqlTimeWithTimeZoneInterval(
                   domain::SqlTimeWithTimeZone{
                       domain::SqlTime{0}, std::numeric_limits<std::int32_t>::max()},
                   domain::SqlInterval{}),
               std::out_of_range);
}
