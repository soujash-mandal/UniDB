#include "../../domain/types/SqlIntegerSemantics.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <variant>

using QueryExecutionService::domain::integerSqlTypeBounds;
using QueryExecutionService::domain::integerValueAsInt64;
using QueryExecutionService::domain::isIntegerSqlType;
using QueryExecutionService::domain::makeSqlIntegerValue;
using unidb::sql::SqlTypeId;

TEST(SqlIntegerSemanticsTest, IdentifiesOnlyTheThreeIntegerTypes) {
  EXPECT_TRUE(isIntegerSqlType(SqlTypeId::SmallInt));
  EXPECT_TRUE(isIntegerSqlType(SqlTypeId::Integer));
  EXPECT_TRUE(isIntegerSqlType(SqlTypeId::BigInt));
  EXPECT_FALSE(isIntegerSqlType(SqlTypeId::Boolean));
  EXPECT_FALSE(isIntegerSqlType(SqlTypeId::Numeric));
  EXPECT_FALSE(isIntegerSqlType(SqlTypeId::Text));
}

TEST(SqlIntegerSemanticsTest, ExposesPostgreSqlSignedIntegerRanges) {
  const auto smallint = integerSqlTypeBounds(SqlTypeId::SmallInt);
  EXPECT_EQ(smallint.minimum, -32768);
  EXPECT_EQ(smallint.maximum, 32767);

  const auto integer = integerSqlTypeBounds(SqlTypeId::Integer);
  EXPECT_EQ(integer.minimum, -2147483648LL);
  EXPECT_EQ(integer.maximum, 2147483647LL);

  const auto bigint = integerSqlTypeBounds(SqlTypeId::BigInt);
  EXPECT_EQ(bigint.minimum, std::numeric_limits<std::int64_t>::min());
  EXPECT_EQ(bigint.maximum, std::numeric_limits<std::int64_t>::max());
}

TEST(SqlIntegerSemanticsTest, CreatesValuesUsingTheDeclaredStorageWidth) {
  const auto smallint = makeSqlIntegerValue(SqlTypeId::SmallInt, -123);
  const auto integer = makeSqlIntegerValue(SqlTypeId::Integer, 123456);
  const auto bigint = makeSqlIntegerValue(SqlTypeId::BigInt, 5000000000LL);

  EXPECT_TRUE(std::holds_alternative<std::int16_t>(smallint));
  EXPECT_TRUE(std::holds_alternative<std::int32_t>(integer));
  EXPECT_TRUE(std::holds_alternative<std::int64_t>(bigint));
  EXPECT_EQ(integerValueAsInt64(smallint), -123);
  EXPECT_EQ(integerValueAsInt64(integer), 123456);
  EXPECT_EQ(integerValueAsInt64(bigint), 5000000000LL);
}

TEST(SqlIntegerSemanticsTest, AcceptsInclusiveMinimumAndMaximumValues) {
  EXPECT_EQ(integerValueAsInt64(makeSqlIntegerValue(SqlTypeId::SmallInt, -32768)),
            -32768);
  EXPECT_EQ(integerValueAsInt64(makeSqlIntegerValue(SqlTypeId::SmallInt, 32767)),
            32767);
  EXPECT_EQ(integerValueAsInt64(
                makeSqlIntegerValue(SqlTypeId::Integer, -2147483648LL)),
            -2147483648LL);
  EXPECT_EQ(integerValueAsInt64(
                makeSqlIntegerValue(SqlTypeId::Integer, 2147483647LL)),
            2147483647LL);
  EXPECT_EQ(integerValueAsInt64(makeSqlIntegerValue(
                SqlTypeId::BigInt, std::numeric_limits<std::int64_t>::min())),
            std::numeric_limits<std::int64_t>::min());
  EXPECT_EQ(integerValueAsInt64(makeSqlIntegerValue(
                SqlTypeId::BigInt, std::numeric_limits<std::int64_t>::max())),
            std::numeric_limits<std::int64_t>::max());
}

TEST(SqlIntegerSemanticsTest, RejectsValuesOutsideTheDeclaredRange) {
  EXPECT_THROW(makeSqlIntegerValue(SqlTypeId::SmallInt, -32769),
               std::out_of_range);
  EXPECT_THROW(makeSqlIntegerValue(SqlTypeId::SmallInt, 32768),
               std::out_of_range);
  EXPECT_THROW(makeSqlIntegerValue(SqlTypeId::Integer, -2147483649LL),
               std::out_of_range);
  EXPECT_THROW(makeSqlIntegerValue(SqlTypeId::Integer, 2147483648LL),
               std::out_of_range);
}

TEST(SqlIntegerSemanticsTest, RejectsNonIntegerSqlTypes) {
  EXPECT_THROW(integerSqlTypeBounds(SqlTypeId::Boolean), std::invalid_argument);
  EXPECT_THROW(makeSqlIntegerValue(SqlTypeId::Text, 1), std::invalid_argument);
}
