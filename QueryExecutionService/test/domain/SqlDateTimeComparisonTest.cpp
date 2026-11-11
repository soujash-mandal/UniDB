#include "../../domain/types/SqlDateTimeComparison.h"

#include <gtest/gtest.h>

#include <compare>
#include <stdexcept>

namespace domain = QueryExecutionService::domain;

namespace {

template <typename Left, typename Right>
std::strong_ordering cmp(const Left &left, const Right &right) {
  return domain::compareSqlTemporalValues(domain::SqlTemporalValue{left},
                                          domain::SqlTemporalValue{right});
}

} // namespace

TEST(SqlDateTimeComparisonTest, ComparesDatesByPostgresEpochDay) {
  EXPECT_EQ(cmp(domain::SqlDate{-1}, domain::SqlDate{0}),
            std::strong_ordering::less);
  EXPECT_EQ(cmp(domain::SqlDate{42}, domain::SqlDate{42}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, ComparesLocalTimesWithoutTimezoneConversion) {
  EXPECT_EQ(cmp(domain::SqlTime{1}, domain::SqlTime{2}),
            std::strong_ordering::less);
}

TEST(SqlDateTimeComparisonTest, ComparesTimeWithTimeZoneAsUtcClockTime) {
  const domain::SqlTimeWithTimeZone noonPlusTwo{
      domain::SqlTime{12LL * 3600 * 1'000'000}, 2 * 3600};
  const domain::SqlTimeWithTimeZone tenUtc{
      domain::SqlTime{10LL * 3600 * 1'000'000}, 0};
  EXPECT_EQ(cmp(noonPlusTwo, tenUtc), std::strong_ordering::less);
}

TEST(SqlDateTimeComparisonTest, WrapsTimeWithTimeZoneAcrossMidnight) {
  const domain::SqlTimeWithTimeZone halfPastMidnightPlusOne{
      domain::SqlTime{30LL * 60 * 1'000'000}, 3600};
  const domain::SqlTimeWithTimeZone quarterToMidnightUtc{
      domain::SqlTime{23LL * 3600 * 1'000'000 + 45LL * 60 * 1'000'000}, 0};
  EXPECT_EQ(cmp(halfPastMidnightPlusOne, quarterToMidnightUtc),
            std::strong_ordering::less);
}

TEST(SqlDateTimeComparisonTest, KeepsLocalTimestampsDistinctFromInstants) {
  const domain::SqlTimestamp earlier{{0}, {10LL * 3600 * 1'000'000}};
  const domain::SqlTimestamp later{{0}, {11LL * 3600 * 1'000'000}};
  EXPECT_EQ(cmp(earlier, later), std::strong_ordering::less);
  EXPECT_THROW(cmp(earlier, domain::SqlTimestampWithTimeZone{0}),
               std::invalid_argument);
}

TEST(SqlDateTimeComparisonTest, ComparesTimestampWithTimeZoneAsAnInstant) {
  EXPECT_EQ(cmp(domain::SqlTimestampWithTimeZone{123456},
                domain::SqlTimestampWithTimeZone{123456}),
            std::strong_ordering::equal);
  EXPECT_EQ(cmp(domain::SqlTimestampWithTimeZone{123457},
                domain::SqlTimestampWithTimeZone{123456}),
            std::strong_ordering::greater);
}

TEST(SqlDateTimeComparisonTest, UsesThirtyDayMonthsForIntervalComparison) {
  EXPECT_EQ(cmp(domain::SqlInterval{1, 0, 0}, domain::SqlInterval{0, 30, 0}),
            std::strong_ordering::equal);
  EXPECT_EQ(cmp(domain::SqlInterval{0, 1, 86'400LL * 1'000'000},
                domain::SqlInterval{0, 2, 0}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, NormalizesNegativeIntervalRemainders) {
  EXPECT_EQ(cmp(domain::SqlInterval{0, -1, 1},
                domain::SqlInterval{0, 0, -86'400LL * 1'000'000 + 1}),
            std::strong_ordering::equal);
}

TEST(SqlDateTimeComparisonTest, RejectsCrossTypeComparisons) {
  EXPECT_THROW(cmp(domain::SqlDate{0}, domain::SqlTime{0}),
               std::invalid_argument);
}
