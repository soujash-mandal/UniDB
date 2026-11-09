#include "../../domain/aggregate/SqlAggregateCalculator.h"
#include "../../domain/types/SqlNumericSemantics.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using QueryExecutionService::domain::calculateAggregate;
using unidb::sql::AggregateFunction;
using unidb::sql::SqlValue;
} // namespace

TEST(SqlAggregateCalculatorTest, CountsRowsIncludingEmptyInput) {
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Count, {}, 7)),
            7);
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Count, {}, 0)),
            0);
}

TEST(SqlAggregateCalculatorTest, CalculatesMinimumAndMaximumForIntegers) {
  const std::vector<SqlValue> values{std::int64_t{8}, std::int64_t{-2},
                                     std::int64_t{5}};
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Min, values, 3)),
            -2);
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Max, values, 3)),
            8);
}

TEST(SqlAggregateCalculatorTest, CalculatesMinimumAndMaximumForText) {
  const std::vector<SqlValue> values{std::string("pear"), std::string("apple"),
                                     std::string("orange")};
  EXPECT_EQ(std::get<std::string>(
                calculateAggregate(AggregateFunction::Min, values, 3)),
            "apple");
  EXPECT_EQ(std::get<std::string>(
                calculateAggregate(AggregateFunction::Max, values, 3)),
            "pear");
}

TEST(SqlAggregateCalculatorTest, CalculatesSumAndTruncatedIntegerAverage) {
  const std::vector<SqlValue> values{std::int64_t{3}, std::int64_t{8},
                                     std::int64_t{4}};
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Sum, values, 3)),
            15);
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Avg, values, 3)),
            5);

  const std::vector<SqlValue> negativeValues{std::int64_t{-4},
                                             std::int64_t{-3}};
  EXPECT_EQ(std::get<std::int64_t>(
                calculateAggregate(AggregateFunction::Avg, negativeValues, 2)),
            -3);
}

TEST(SqlAggregateCalculatorTest, CalculatesFloatingPointSumAndAverage) {
  const std::vector<SqlValue> realValues{1.5F, 2.25F, -0.5F};
  const auto realSum =
      calculateAggregate(AggregateFunction::Sum, realValues, 3);
  const auto realAverage =
      calculateAggregate(AggregateFunction::Avg, realValues, 3);
  EXPECT_FLOAT_EQ(std::get<float>(realSum), 3.25F);
  EXPECT_DOUBLE_EQ(std::get<double>(realAverage), 3.25 / 3.0);

  const std::vector<SqlValue> doubleValues{1.5, 2.25, -0.5};
  const auto doubleSum =
      calculateAggregate(AggregateFunction::Sum, doubleValues, 3);
  const auto doubleAverage =
      calculateAggregate(AggregateFunction::Avg, doubleValues, 3);
  EXPECT_DOUBLE_EQ(std::get<double>(doubleSum), 3.25);
  EXPECT_DOUBLE_EQ(std::get<double>(doubleAverage), 3.25 / 3.0);
}

TEST(SqlAggregateCalculatorTest, PreservesFloatingPointNonFiniteResults) {
  const std::vector<SqlValue> values{std::numeric_limits<double>::infinity(),
                                     1.0};
  const auto sum = calculateAggregate(AggregateFunction::Sum, values, 2);
  EXPECT_TRUE(std::isinf(std::get<double>(sum)));

  const std::vector<SqlValue> nanValues{std::numeric_limits<float>::quiet_NaN(),
                                        1.0F};
  const auto average = calculateAggregate(AggregateFunction::Avg, nanValues, 2);
  EXPECT_TRUE(std::isnan(std::get<double>(average)));
}

TEST(SqlAggregateCalculatorTest, RejectsSumOverflow) {
  const std::vector<SqlValue> positiveOverflow{
      std::numeric_limits<std::int64_t>::max(), std::int64_t{1}};
  EXPECT_THROW(calculateAggregate(AggregateFunction::Sum, positiveOverflow, 2),
               std::invalid_argument);

  const std::vector<SqlValue> negativeOverflow{
      std::numeric_limits<std::int64_t>::min(), std::int64_t{-1}};
  EXPECT_THROW(calculateAggregate(AggregateFunction::Sum, negativeOverflow, 2),
               std::invalid_argument);
}

TEST(SqlAggregateCalculatorTest, RejectsNonCountAggregatesOnEmptyInput) {
  for (const auto function : {AggregateFunction::Min, AggregateFunction::Max,
                              AggregateFunction::Sum, AggregateFunction::Avg}) {
    EXPECT_THROW(calculateAggregate(function, {}, 0), std::invalid_argument);
  }
}

TEST(SqlAggregateCalculatorTest, CalculatesExactNumericSumAcrossScales) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{false, "12345", 2},
      unidb::sql::SqlNumericValue{false, "55", 1},
      unidb::sql::SqlNumericValue{true, "25", 2}};
  const auto sum = calculateAggregate(AggregateFunction::Sum, values, 3);
  const auto &numeric = std::get<unidb::sql::SqlNumericValue>(sum);
  EXPECT_EQ(numeric, (unidb::sql::SqlNumericValue{false, "1287", 1}));
}

TEST(SqlAggregateCalculatorTest, CalculatesExactNumericSumWithCancellation) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{true, "999", 2},
      unidb::sql::SqlNumericValue{false, "999", 2},
      unidb::sql::SqlNumericValue{false, "1", 0}};
  const auto sum = calculateAggregate(AggregateFunction::Sum, values, 3);
  EXPECT_EQ(std::get<unidb::sql::SqlNumericValue>(sum),
            (unidb::sql::SqlNumericValue{false, "1", 0}));
}

TEST(SqlAggregateCalculatorTest,
     PreservesLargeNumericSumWithoutNativeOverflow) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{false, "999999999999999999999999999999", 4},
      unidb::sql::SqlNumericValue{false, "1", 4}};
  const auto sum = calculateAggregate(AggregateFunction::Sum, values, 2);
  EXPECT_EQ(std::get<unidb::sql::SqlNumericValue>(sum),
            (unidb::sql::SqlNumericValue{false, "1", -26}));
}

TEST(SqlAggregateCalculatorTest, CalculatesExactNumericAverage) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{false, "1", 0},
      unidb::sql::SqlNumericValue{false, "2", 0}};
  const auto average = calculateAggregate(AggregateFunction::Avg, values, 2);
  EXPECT_EQ(std::get<unidb::sql::SqlNumericValue>(average),
            (unidb::sql::SqlNumericValue{false, "15", 1}));
}

TEST(SqlAggregateCalculatorTest,
     CalculatesRepeatingNumericAverageAtSixteenPlaces) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{false, "1", 0},
      unidb::sql::SqlNumericValue{false, "0", 0},
      unidb::sql::SqlNumericValue{false, "0", 0}};
  const auto average = calculateAggregate(AggregateFunction::Avg, values, 3);
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<unidb::sql::SqlNumericValue>(average)),
            "0.3333333333333333");
}

TEST(SqlAggregateCalculatorTest, PreservesNegativeNumericAverage) {
  const std::vector<SqlValue> values{unidb::sql::SqlNumericValue{true, "1", 0},
                                     unidb::sql::SqlNumericValue{true, "2", 0}};
  const auto average = calculateAggregate(AggregateFunction::Avg, values, 2);
  EXPECT_EQ(unidb::sql::SqlNumericValue(
                std::get<unidb::sql::SqlNumericValue>(average)),
            (unidb::sql::SqlNumericValue{true, "15", 1}));
}

TEST(SqlAggregateCalculatorTest, PreservesLargeNumericAverageExactly) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{false, "999999999999999999999999999999", 4},
      unidb::sql::SqlNumericValue{false, "999999999999999999999999999999", 4}};
  const auto average = calculateAggregate(AggregateFunction::Avg, values, 2);
  EXPECT_EQ(std::get<unidb::sql::SqlNumericValue>(average),
            (unidb::sql::SqlNumericValue{false,
                                         "999999999999999999999999999999", 4}));
}

TEST(SqlAggregateCalculatorTest, RejectsMixedNumericAndNonNumericSumValues) {
  const std::vector<SqlValue> values{
      unidb::sql::SqlNumericValue{false, "10", 1}, std::int64_t{2}};
  EXPECT_THROW(calculateAggregate(AggregateFunction::Sum, values, 2),
               std::invalid_argument);
}
