#include "../../domain/types/SqlIntervalArithmetic.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace domain = QueryExecutionService::domain;

TEST(SqlIntervalArithmeticTest, AddsComponentsWithoutConvertingMonthsToDays) {
  const auto result = domain::addSqlIntervals(domain::SqlInterval{2, 3, 4},
                                              domain::SqlInterval{5, 7, 11});

  EXPECT_EQ(result, (domain::SqlInterval{7, 10, 15}));
}

TEST(SqlIntervalArithmeticTest, PreservesMixedSignComponents) {
  const auto result = domain::addSqlIntervals(domain::SqlInterval{2, -3, 4},
                                              domain::SqlInterval{-5, 7, -11});

  EXPECT_EQ(result, (domain::SqlInterval{-3, 4, -7}));
}

TEST(SqlIntervalArithmeticTest, SubtractsComponentsIndependently) {
  const auto result = domain::subtractSqlIntervals(
      domain::SqlInterval{8, 3, -4}, domain::SqlInterval{5, -7, -11});

  EXPECT_EQ(result, (domain::SqlInterval{3, 10, 7}));
}

TEST(SqlIntervalArithmeticTest, NegatesEveryComponent) {
  const auto result = domain::negateSqlInterval(domain::SqlInterval{2, -3, 4});

  EXPECT_EQ(result, (domain::SqlInterval{-2, 3, -4}));
}

TEST(SqlIntervalArithmeticTest, SupportsZeroIntervals) {
  const auto zero = domain::SqlInterval{};
  EXPECT_EQ(domain::addSqlIntervals(zero, zero), zero);
  EXPECT_EQ(domain::subtractSqlIntervals(zero, zero), zero);
  EXPECT_EQ(domain::negateSqlInterval(zero), zero);
}

TEST(SqlIntervalArithmeticTest, RejectsAdditionOverflowInEveryComponent) {
  constexpr auto int32Max = std::numeric_limits<std::int32_t>::max();
  constexpr auto int64Max = std::numeric_limits<std::int64_t>::max();

  EXPECT_THROW(domain::addSqlIntervals(domain::SqlInterval{int32Max, 0, 0},
                                       domain::SqlInterval{1, 0, 0}),
               std::out_of_range);
  EXPECT_THROW(domain::addSqlIntervals(domain::SqlInterval{0, int32Max, 0},
                                       domain::SqlInterval{0, 1, 0}),
               std::out_of_range);
  EXPECT_THROW(domain::addSqlIntervals(domain::SqlInterval{0, 0, int64Max},
                                       domain::SqlInterval{0, 0, 1}),
               std::out_of_range);
}

TEST(SqlIntervalArithmeticTest, RejectsSubtractionOverflowInEveryComponent) {
  constexpr auto int32Min = std::numeric_limits<std::int32_t>::min();
  constexpr auto int64Min = std::numeric_limits<std::int64_t>::min();

  EXPECT_THROW(domain::subtractSqlIntervals(domain::SqlInterval{int32Min, 0, 0},
                                            domain::SqlInterval{1, 0, 0}),
               std::out_of_range);
  EXPECT_THROW(domain::subtractSqlIntervals(domain::SqlInterval{0, int32Min, 0},
                                            domain::SqlInterval{0, 1, 0}),
               std::out_of_range);
  EXPECT_THROW(domain::subtractSqlIntervals(domain::SqlInterval{0, 0, int64Min},
                                            domain::SqlInterval{0, 0, 1}),
               std::out_of_range);
}

TEST(SqlIntervalArithmeticTest, RejectsNegatingMinimumComponentValues) {
  constexpr auto int32Min = std::numeric_limits<std::int32_t>::min();
  constexpr auto int64Min = std::numeric_limits<std::int64_t>::min();

  EXPECT_THROW(domain::negateSqlInterval(domain::SqlInterval{int32Min, 0, 0}),
               std::out_of_range);
  EXPECT_THROW(domain::negateSqlInterval(domain::SqlInterval{0, int32Min, 0}),
               std::out_of_range);
  EXPECT_THROW(domain::negateSqlInterval(domain::SqlInterval{0, 0, int64Min}),
               std::out_of_range);
}
