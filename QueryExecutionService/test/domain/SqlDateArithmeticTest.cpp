#include "../../domain/types/SqlDateArithmetic.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace domain = QueryExecutionService::domain;

TEST(SqlDateArithmeticTest, AddsCalendarDaysInBothOperandOrders) {
  const domain::SqlDate epoch{0};

  EXPECT_EQ(domain::addSqlDateDays(epoch, 1), domain::SqlDate{1});
  EXPECT_EQ(domain::addSqlDateDays(1, epoch), domain::SqlDate{1});
  EXPECT_EQ(domain::addSqlDateDays(epoch, -1), domain::SqlDate{-1});
}

TEST(SqlDateArithmeticTest, SubtractsCalendarDaysIncludingNegativeOffsets) {
  const domain::SqlDate epoch{0};

  EXPECT_EQ(domain::subtractSqlDateDays(epoch, 1), domain::SqlDate{-1});
  EXPECT_EQ(domain::subtractSqlDateDays(epoch, -1), domain::SqlDate{1});
  EXPECT_EQ(domain::subtractSqlDateDays(epoch, 0), epoch);
}

TEST(SqlDateArithmeticTest, DateDifferenceIsSignedAndUsesCalendarDays) {
  EXPECT_EQ(domain::differenceSqlDates(domain::SqlDate{10}, domain::SqlDate{3}),
            7);
  EXPECT_EQ(domain::differenceSqlDates(domain::SqlDate{3}, domain::SqlDate{10}),
            -7);
  EXPECT_EQ(
      domain::differenceSqlDates(domain::SqlDate{42}, domain::SqlDate{42}), 0);
}

TEST(SqlDateArithmeticTest, ArithmeticCrossesLeapDayAndPostgresEpoch) {
  const auto dayBeforeLeapDay = domain::makeSqlDate(2024, 2, 28);
  const auto leapDay = domain::makeSqlDate(2024, 2, 29);
  const auto dayAfterLeapDay = domain::makeSqlDate(2024, 3, 1);

  EXPECT_EQ(domain::addSqlDateDays(dayBeforeLeapDay, 1), leapDay);
  EXPECT_EQ(domain::addSqlDateDays(leapDay, 1), dayAfterLeapDay);
  EXPECT_EQ(domain::differenceSqlDates(dayAfterLeapDay, dayBeforeLeapDay), 2);

  const auto dayBeforeEpoch = domain::makeSqlDate(1999, 12, 31);
  EXPECT_EQ(domain::addSqlDateDays(dayBeforeEpoch, 1), domain::SqlDate{0});
}

TEST(SqlDateArithmeticTest, RejectsResultsOutsidePostgresDateRange) {
  constexpr domain::SqlDate minimum{-2'451'545};
  constexpr domain::SqlDate maximum{2'145'031'948};

  EXPECT_THROW(domain::addSqlDateDays(minimum, -1), std::out_of_range);
  EXPECT_THROW(domain::subtractSqlDateDays(minimum, 1), std::out_of_range);
  EXPECT_THROW(domain::addSqlDateDays(maximum, 1), std::out_of_range);
  EXPECT_THROW(domain::subtractSqlDateDays(maximum, -1), std::out_of_range);
}

TEST(SqlDateArithmeticTest, HandlesExtremeIntegerOffsetsWithoutOverflow) {
  constexpr domain::SqlDate epoch{0};

  EXPECT_THROW(
      domain::addSqlDateDays(epoch, std::numeric_limits<std::int32_t>::min()),
      std::out_of_range);
  EXPECT_THROW(
      domain::addSqlDateDays(epoch, std::numeric_limits<std::int32_t>::max()),
      std::out_of_range);
  EXPECT_THROW(domain::subtractSqlDateDays(
                   epoch, std::numeric_limits<std::int32_t>::min()),
               std::out_of_range);
  EXPECT_THROW(domain::subtractSqlDateDays(
                   epoch, std::numeric_limits<std::int32_t>::max()),
               std::out_of_range);
}

TEST(SqlDateArithmeticTest,
     DateDifferenceFitsPostgresIntegerAtFiniteBoundaries) {
  constexpr domain::SqlDate minimum{-2'451'545};
  constexpr domain::SqlDate maximum{2'145'031'948};

  EXPECT_EQ(domain::differenceSqlDates(maximum, minimum), 2'147'483'493);
  EXPECT_EQ(domain::differenceSqlDates(minimum, maximum), -2'147'483'493);
}
