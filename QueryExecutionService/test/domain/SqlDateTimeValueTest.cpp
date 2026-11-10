#include "../../domain/types/SqlDateTimeValue.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace domain = QueryExecutionService::domain;

using domain::SqlDate;
using domain::SqlInterval;
using domain::SqlTime;
using domain::SqlTimeWithTimeZone;
using domain::SqlTimestamp;
using domain::SqlTimestampWithTimeZone;

static_assert(!std::is_convertible_v<SqlDate, SqlTime>);
static_assert(!std::is_convertible_v<SqlTime, SqlTimestamp>);
static_assert(!std::is_convertible_v<SqlTimestamp, SqlTimestampWithTimeZone>);

TEST(SqlDateTimeValueTest, DateStoresSignedDaysFromPostgresEpoch) {
  EXPECT_EQ(SqlDate{0}.daysSinceEpoch, 0);
  EXPECT_EQ(SqlDate{-1}.daysSinceEpoch, -1);
  EXPECT_EQ(SqlDate{1}.daysSinceEpoch, 1);
  EXPECT_EQ(SqlDate{42}, SqlDate{42});
}

TEST(SqlDateTimeValueTest, TimeStoresMicrosecondsSinceLocalMidnight) {
  constexpr std::int64_t microsecondsPerDay = 24LL * 60LL * 60LL * 1000000LL;

  EXPECT_EQ(SqlTime{0}.microsecondsSinceMidnight, 0);
  EXPECT_EQ(SqlTime{microsecondsPerDay}.microsecondsSinceMidnight,
            microsecondsPerDay);
  EXPECT_NE(SqlTime{0}, SqlTime{microsecondsPerDay});
}

TEST(SqlDateTimeValueTest, TimeWithTimeZoneKeepsTimeAndOffsetSeparate) {
  const SqlTimeWithTimeZone value{SqlTime{12LL * 60LL * 60LL * 1000000LL},
                                  5 * 60 * 60 + 30 * 60};

  EXPECT_EQ(value.time.microsecondsSinceMidnight,
            12LL * 60LL * 60LL * 1000000LL);
  EXPECT_EQ(value.offsetSecondsEastOfUtc, 19800);
}

TEST(SqlDateTimeValueTest, TimestampKeepsLocalDateAndTimeComponents) {
  const SqlTimestamp value{SqlDate{10}, SqlTime{123456789}};

  EXPECT_EQ(value.date, SqlDate{10});
  EXPECT_EQ(value.time, SqlTime{123456789});
}

TEST(SqlDateTimeValueTest, TimestampWithTimeZoneStoresAnInstant) {
  const SqlTimestampWithTimeZone beforeEpoch{-1};
  const SqlTimestampWithTimeZone atEpoch{0};
  const SqlTimestampWithTimeZone afterEpoch{1};

  EXPECT_LT(beforeEpoch.microsecondsSinceEpoch, atEpoch.microsecondsSinceEpoch);
  EXPECT_LT(atEpoch.microsecondsSinceEpoch, afterEpoch.microsecondsSinceEpoch);
}

TEST(SqlDateTimeValueTest, IntervalKeepsCalendarAndTimeComponentsDistinct) {
  const SqlInterval oneMonth{.months = 1};
  const SqlInterval thirtyDays{.days = 30};
  const SqlInterval oneDayAndOneSecond{.days = 1, .microseconds = 1000000};

  EXPECT_NE(oneMonth, thirtyDays);
  EXPECT_EQ(oneMonth.months, 1);
  EXPECT_EQ(oneMonth.days, 0);
  EXPECT_EQ(oneDayAndOneSecond.days, 1);
  EXPECT_EQ(oneDayAndOneSecond.microseconds, 1000000);
}

TEST(SqlDateTimeValueTest, ValueTypesSupportStructuralEquality) {
  EXPECT_EQ((SqlTimeWithTimeZone{SqlTime{100}, 3600}),
            (SqlTimeWithTimeZone{SqlTime{100}, 3600}));
  EXPECT_EQ((SqlTimestamp{SqlDate{2}, SqlTime{3}}),
            (SqlTimestamp{SqlDate{2}, SqlTime{3}}));
  EXPECT_EQ(SqlTimestampWithTimeZone{123}, SqlTimestampWithTimeZone{123});
  EXPECT_EQ((SqlInterval{1, 2, 3}), (SqlInterval{1, 2, 3}));
}
