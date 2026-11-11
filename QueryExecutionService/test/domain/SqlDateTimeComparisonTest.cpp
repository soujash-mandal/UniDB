#include "../../domain/types/SqlDateTimeComparison.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace domain = QueryExecutionService::domain;

TEST(SqlDateTimeComparisonTest, ComparesDatesByPostgresEpochDay) {
  EXPECT_LT(
      domain::compareSqlTemporalValues(domain::SqlDate{-1}, domain::SqlDate{0}),
      std::strong_ordering::equal);
  EXPECT_EQ(domain::compareSqlTemporalValues(domain::SqlDate{42},
                                             domain::SqlDate{42}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, ComparesLocalTimesWithoutTimezoneConversion) {
  EXPECT_LT(
      domain::compareSqlTemporalValues(domain::SqlTime{1}, domain::SqlTime{2}),
      std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, ComparesTimeWithTimeZoneAsUtcClockTime) {
  const domain::SqlTimeWithTimeZone noonPlusTwo{
      domain::SqlTime{12LL * 3600 * 1'000'000}, 2 * 3600};
  const domain::SqlTimeWithTimeZone tenUtc{
      domain::SqlTime{10LL * 3600 * 1'000'000}, 0};
  EXPECT_EQ(domain::compareSqlTemporalValues(noonPlusTwo, tenUtc),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, WrapsTimeWithTimeZoneAcrossMidnight) {
  const domain::SqlTimeWithTimeZone halfPastMidnightPlusOne{
      domain::SqlTime{30LL * 60 * 1'000'000}, 3600};
  const domain::SqlTimeWithTimeZone quarterToMidnightUtc{
      domain::SqlTime{23LL * 3600 * 1'000'000 + 45LL * 60 * 1'000'000}, 0};
  EXPECT_LT(domain::compareSqlTemporalValues(halfPastMidnightPlusOne,
                                             quarterToMidnightUtc),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, KeepsLocalTimestampsDistinctFromInstants) {
  const domain::SqlTimestamp earlier{{0}, {10LL * 3600 * 1'000'000}};
  const domain::SqlTimestamp later{{0}, {11LL * 3600 * 1'000'000}};
  EXPECT_LT(domain::compareSqlTemporalValues(earlier, later),
            std::strong_ordering::equal);

  EXPECT_THROW(
      domain::compareSqlTemporalValues(
          domain::SqlTemporalValue{earlier},
          domain::SqlTemporalValue{domain::SqlTimestampWithTimeZone{0}}),
      std::invalid_argument);
}

TEST(SqlDateTimeComparisonTest, ComparesTimestampWithTimeZoneAsAnInstant) {
  EXPECT_EQ(domain::compareSqlTemporalValues(
                domain::SqlTimestampWithTimeZone{123456},
                domain::SqlTimestampWithTimeZone{123456}),
            std::strong_ordering::equal);
  EXPECT_GT(domain::compareSqlTemporalValues(
                domain::SqlTimestampWithTimeZone{123457},
                domain::SqlTimestampWithTimeZone{123456}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, UsesThirtyDayMonthsForIntervalComparison) {
  EXPECT_EQ(domain::compareSqlTemporalValues(domain::SqlInterval{1, 0, 0},
                                             domain::SqlInterval{0, 30, 0}),
            std::strong_ordering::equal);
  EXPECT_EQ(domain::compareSqlTemporalValues(
                domain::SqlInterval{0, 1, 86'400LL * 1'000'000},
                domain::SqlInterval{0, 2, 0}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, NormalizesNegativeIntervalRemainders) {
  EXPECT_EQ(domain::compareSqlTemporalValues(
                domain::SqlInterval{0, -1, 1},
                domain::SqlInterval{0, 0, -86'400LL * 1'000'000 + 1}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, RejectsCrossTypeComparisons) {
  EXPECT_THROW(
      domain::compareSqlTemporalValues(domain::SqlDate{0}, domain::SqlTime{0}),
      std::invalid_argument);
}
