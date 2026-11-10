#include "../../domain/types/SqlNumericSemantics.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

using QueryExecutionService::domain::addSqlNumericValues;
using QueryExecutionService::domain::compareSqlNumericValues;
using QueryExecutionService::domain::divideSqlNumericValues;
using QueryExecutionService::domain::formatSqlNumericValue;
using QueryExecutionService::domain::multiplySqlNumericValues;
using QueryExecutionService::domain::normalizeSqlNumericValue;
using QueryExecutionService::domain::parseSqlNumericLiteral;
using QueryExecutionService::domain::selectSqlNumericDivisionScale;
using QueryExecutionService::domain::subtractSqlNumericValues;

TEST(SqlNumericSemanticsTest, NormalizesLeadingAndTrailingZeroesExactly) {
  EXPECT_EQ(normalizeSqlNumericValue(false, "0001234500", 4),
            (unidb::sql::SqlNumericValue{false, "12345", 2}));
  EXPECT_EQ(normalizeSqlNumericValue(true, "0001200", 0),
            (unidb::sql::SqlNumericValue{true, "12", -2}));
}

TEST(SqlNumericSemanticsTest, NormalizesEveryRepresentationOfZero) {
  EXPECT_EQ(normalizeSqlNumericValue(true, "000000", 12),
            (unidb::sql::SqlNumericValue{false, "0", 0}));
}

TEST(SqlNumericSemanticsTest, RejectsEmptyAndNonDigitCoefficients) {
  EXPECT_THROW(normalizeSqlNumericValue(false, "", 0), std::invalid_argument);
  EXPECT_THROW(normalizeSqlNumericValue(false, "12.3", 1),
               std::invalid_argument);
  EXPECT_THROW(normalizeSqlNumericValue(false, "12a3", 0),
               std::invalid_argument);
}

TEST(SqlNumericSemanticsTest, RejectsScaleUnderflowWhenRemovingTrailingZeroes) {
  EXPECT_THROW(normalizeSqlNumericValue(
                   false, "10", std::numeric_limits<std::int32_t>::min()),
               std::out_of_range);
}

TEST(SqlNumericSemanticsTest, FormatsPositiveNegativeAndScaledValues) {
  EXPECT_EQ(formatSqlNumericValue(normalizeSqlNumericValue(false, "12345", 2)),
            "123.45");
  EXPECT_EQ(formatSqlNumericValue(normalizeSqlNumericValue(true, "12345", 2)),
            "-123.45");
  EXPECT_EQ(formatSqlNumericValue(normalizeSqlNumericValue(false, "12", 4)),
            "0.0012");
  EXPECT_EQ(formatSqlNumericValue(normalizeSqlNumericValue(false, "1200", 0)),
            "1200");
  EXPECT_EQ(formatSqlNumericValue(normalizeSqlNumericValue(false, "123", -2)),
            "12300");
}

TEST(SqlNumericSemanticsTest, ComparesDifferentScalesExactly) {
  const auto onePointTwo = normalizeSqlNumericValue(false, "1200", 3);
  const auto onePointNineteen = normalizeSqlNumericValue(false, "119", 2);
  const auto onePointTwoZero = normalizeSqlNumericValue(false, "120", 2);

  EXPECT_GT(compareSqlNumericValues(onePointTwo, onePointNineteen), 0);
  EXPECT_EQ(compareSqlNumericValues(onePointTwo, onePointTwoZero), 0);
}

TEST(SqlNumericSemanticsTest, ComparesNegativeAndZeroValues) {
  const auto negativeTwo = normalizeSqlNumericValue(true, "200", 2);
  const auto negativeOne = normalizeSqlNumericValue(true, "100", 2);
  const auto zero = normalizeSqlNumericValue(false, "0", 0);
  const auto positiveOne = normalizeSqlNumericValue(false, "100", 2);

  EXPECT_LT(compareSqlNumericValues(negativeTwo, negativeOne), 0);
  EXPECT_LT(compareSqlNumericValues(negativeOne, zero), 0);
  EXPECT_LT(compareSqlNumericValues(zero, positiveOne), 0);
}

TEST(SqlNumericSemanticsTest, ComparesLargeValuesWithoutMachineIntegerLimits) {
  const auto larger =
      normalizeSqlNumericValue(false, "999999999999999999999999999999", 0);
  const auto smaller =
      normalizeSqlNumericValue(false, "100000000000000000000000000000", 0);

  EXPECT_GT(compareSqlNumericValues(larger, smaller), 0);
}

TEST(SqlNumericSemanticsTest, RejectsNonCanonicalValuesAtPublicBoundaries) {
  EXPECT_THROW(formatSqlNumericValue({false, "001", 0}), std::invalid_argument);
  EXPECT_THROW(compareSqlNumericValues({true, "0", 0}, {false, "1", 0}),
               std::invalid_argument);
}

TEST(SqlNumericSemanticsTest, ParsesDecimalLiteralsWithoutFloatingPointLoss) {
  EXPECT_EQ(parseSqlNumericLiteral("123.4500"),
            (unidb::sql::SqlNumericValue{false, "12345", 2}));
  EXPECT_EQ(parseSqlNumericLiteral("-0.00120"),
            (unidb::sql::SqlNumericValue{true, "12", 4}));
  EXPECT_EQ(parseSqlNumericLiteral(".5"),
            (unidb::sql::SqlNumericValue{false, "5", 1}));
  EXPECT_EQ(parseSqlNumericLiteral("5."),
            (unidb::sql::SqlNumericValue{false, "5", 0}));
}

TEST(SqlNumericSemanticsTest, ParsesExponentNotationExactly) {
  EXPECT_EQ(parseSqlNumericLiteral("1.25e2"),
            (unidb::sql::SqlNumericValue{false, "125", 0}));
  EXPECT_EQ(parseSqlNumericLiteral("1e-3"),
            (unidb::sql::SqlNumericValue{false, "1", 3}));
  EXPECT_EQ(parseSqlNumericLiteral("-12.50E+2"),
            (unidb::sql::SqlNumericValue{true, "125", -1}));
}

TEST(SqlNumericSemanticsTest, RejectsMalformedNumericLiteralLexemes) {
  for (const auto *literal :
       {"", "+", "-", ".", "1e", "1e+", "1.2.3", "1a", "1e2.3"}) {
    EXPECT_THROW(parseSqlNumericLiteral(literal), std::invalid_argument)
        << literal;
  }
}

TEST(SqlNumericSemanticsTest, RejectsNumericLiteralExponentOverflow) {
  EXPECT_THROW(parseSqlNumericLiteral("1e999999999999999999999"),
               std::out_of_range);
}

TEST(SqlNumericSemanticsTest, AddsExactValuesAcrossDifferentScales) {
  EXPECT_EQ(
      formatSqlNumericValue(addSqlNumericValues(
          parseSqlNumericLiteral("12.34"), parseSqlNumericLiteral("0.006"))),
      "12.346");
  EXPECT_EQ(
      formatSqlNumericValue(addSqlNumericValues(
          parseSqlNumericLiteral("-12.34"), parseSqlNumericLiteral("2.34"))),
      "-10");
}

TEST(SqlNumericSemanticsTest, AddsOppositeSignsAndCancelsToCanonicalZero) {
  EXPECT_EQ(addSqlNumericValues(parseSqlNumericLiteral("1.25"),
                                parseSqlNumericLiteral("-1.25")),
            (unidb::sql::SqlNumericValue{false, "0", 0}));
  EXPECT_EQ(formatSqlNumericValue(addSqlNumericValues(
                parseSqlNumericLiteral("-2"), parseSqlNumericLiteral("-3"))),
            "-5");
}

TEST(SqlNumericSemanticsTest, SubtractsExactValuesAcrossDifferentScales) {
  EXPECT_EQ(
      formatSqlNumericValue(subtractSqlNumericValues(
          parseSqlNumericLiteral("100.01"), parseSqlNumericLiteral("0.99"))),
      "99.02");
  EXPECT_EQ(formatSqlNumericValue(subtractSqlNumericValues(
                parseSqlNumericLiteral("1"), parseSqlNumericLiteral("3"))),
            "-2");
}

TEST(SqlNumericSemanticsTest,
     MultipliesExactValuesWithoutMachineIntegerLimits) {
  EXPECT_EQ(
      formatSqlNumericValue(multiplySqlNumericValues(
          parseSqlNumericLiteral("12.5"), parseSqlNumericLiteral("-0.04"))),
      "-0.5");
  EXPECT_EQ(formatSqlNumericValue(multiplySqlNumericValues(
                parseSqlNumericLiteral("999999999999999999999"),
                parseSqlNumericLiteral("999999999999999999999"))),
            "999999999999999999998000000000000000000001");
  EXPECT_EQ(formatSqlNumericValue(multiplySqlNumericValues(
                parseSqlNumericLiteral("1e3"), parseSqlNumericLiteral("2e-2"))),
            "20");
}

TEST(SqlNumericSemanticsTest, DividesExactlyAtTheRequestedScale) {
  EXPECT_EQ(formatSqlNumericValue(divideSqlNumericValues(
                parseSqlNumericLiteral("1"), parseSqlNumericLiteral("3"), 4)),
            "0.3333");
  EXPECT_EQ(formatSqlNumericValue(divideSqlNumericValues(
                parseSqlNumericLiteral("1"), parseSqlNumericLiteral("8"), 2)),
            "0.13");
  EXPECT_EQ(formatSqlNumericValue(divideSqlNumericValues(
                parseSqlNumericLiteral("-1"), parseSqlNumericLiteral("8"), 2)),
            "-0.13");
  EXPECT_EQ(formatSqlNumericValue(divideSqlNumericValues(
                parseSqlNumericLiteral("100"), parseSqlNumericLiteral("4"), 0)),
            "25");
}

TEST(SqlNumericSemanticsTest, DivisionUsesScaleAwareExactArithmetic) {
  EXPECT_EQ(
      formatSqlNumericValue(divideSqlNumericValues(
          parseSqlNumericLiteral("1.2"), parseSqlNumericLiteral("0.03"), 3)),
      "40");
  EXPECT_EQ(formatSqlNumericValue(divideSqlNumericValues(
                parseSqlNumericLiteral("1"), parseSqlNumericLiteral("6"), 2)),
            "0.17");
}

TEST(SqlNumericSemanticsTest, RejectsDivisionByZeroAndNonCanonicalInputs) {
  EXPECT_THROW(divideSqlNumericValues(parseSqlNumericLiteral("1"),
                                      parseSqlNumericLiteral("0"), 4),
               std::invalid_argument);
  EXPECT_THROW(
      divideSqlNumericValues({false, "01", 0}, parseSqlNumericLiteral("2"), 4),
      std::invalid_argument);
}

TEST(SqlNumericSemanticsTest, RejectsNonCanonicalArithmeticInputs) {
  EXPECT_THROW(
      addSqlNumericValues({false, "01", 0}, parseSqlNumericLiteral("1")),
      std::invalid_argument);
}

TEST(SqlNumericSemanticsTest,
     SelectsPostgresStyleDivisionScaleFromResultMagnitude) {
  EXPECT_EQ(selectSqlNumericDivisionScale(parseSqlNumericLiteral("1"),
                                          parseSqlNumericLiteral("3")),
            20);
  EXPECT_EQ(selectSqlNumericDivisionScale(parseSqlNumericLiteral("5"),
                                          parseSqlNumericLiteral("2")),
            16);
  EXPECT_EQ(selectSqlNumericDivisionScale(parseSqlNumericLiteral("1000000"),
                                          parseSqlNumericLiteral("3")),
            12);
  EXPECT_EQ(selectSqlNumericDivisionScale(parseSqlNumericLiteral("0.001"),
                                          parseSqlNumericLiteral("3")),
            20);
  EXPECT_EQ(selectSqlNumericDivisionScale(parseSqlNumericLiteral("1e-20"),
                                          parseSqlNumericLiteral("3")),
            40);
}
