#include "../../domain/types/SqlTypeRegistry.h"

#include <gtest/gtest.h>

#include <stdexcept>

using QueryExecutionService::domain::canonicalSqlTypeName;
using QueryExecutionService::domain::resolveSqlType;
using unidb::sql::SqlTypeId;

TEST(SqlTypeRegistryTest, ResolvesIntegerAliasesToCanonicalIdentities) {
  EXPECT_EQ(resolveSqlType("smallint").id, SqlTypeId::SmallInt);
  EXPECT_EQ(resolveSqlType("INT2").id, SqlTypeId::SmallInt);
  EXPECT_EQ(resolveSqlType("int").id, SqlTypeId::Integer);
  EXPECT_EQ(resolveSqlType("INTEGER").id, SqlTypeId::Integer);
  EXPECT_EQ(resolveSqlType("int4").id, SqlTypeId::Integer);
  EXPECT_EQ(resolveSqlType("bigint").id, SqlTypeId::BigInt);
  EXPECT_EQ(resolveSqlType("int8").id, SqlTypeId::BigInt);
}

TEST(SqlTypeRegistryTest, ResolvesFloatingPointAndBooleanAliases) {
  EXPECT_EQ(resolveSqlType("real").id, SqlTypeId::Real);
  EXPECT_EQ(resolveSqlType("float4").id, SqlTypeId::Real);
  EXPECT_EQ(resolveSqlType("double   precision").id,
            SqlTypeId::DoublePrecision);
  EXPECT_EQ(resolveSqlType("float8").id, SqlTypeId::DoublePrecision);
  EXPECT_EQ(resolveSqlType("bool").id, SqlTypeId::Boolean);
  EXPECT_EQ(resolveSqlType("BOOLEAN").id, SqlTypeId::Boolean);
}

TEST(SqlTypeRegistryTest, ResolvesCharacterAndTemporalAliases) {
  EXPECT_EQ(resolveSqlType("char").id, SqlTypeId::Char);
  EXPECT_EQ(resolveSqlType("character").id, SqlTypeId::Char);
  EXPECT_EQ(resolveSqlType("varchar").id, SqlTypeId::Varchar);
  EXPECT_EQ(resolveSqlType("character varying").id, SqlTypeId::Varchar);
  EXPECT_EQ(resolveSqlType("time").id, SqlTypeId::Time);
  EXPECT_EQ(resolveSqlType("timetz").id, SqlTypeId::TimeWithTimeZone);
  EXPECT_EQ(resolveSqlType("timestamp").id, SqlTypeId::Timestamp);
  EXPECT_EQ(resolveSqlType("timestamptz").id, SqlTypeId::TimestampWithTimeZone);
}

TEST(SqlTypeRegistryTest, ResolvesOtherCoreTypeNames) {
  EXPECT_EQ(resolveSqlType("decimal").id, SqlTypeId::Numeric);
  EXPECT_EQ(resolveSqlType("date").id, SqlTypeId::Date);
  EXPECT_EQ(resolveSqlType("interval").id, SqlTypeId::Interval);
  EXPECT_EQ(resolveSqlType("bytea").id, SqlTypeId::Bytea);
  EXPECT_EQ(resolveSqlType("uuid").id, SqlTypeId::Uuid);
  EXPECT_EQ(resolveSqlType("json").id, SqlTypeId::Json);
  EXPECT_EQ(resolveSqlType("jsonb").id, SqlTypeId::Jsonb);
}

TEST(SqlTypeRegistryTest, CanonicalNamesAreStable) {
  EXPECT_EQ(canonicalSqlTypeName(SqlTypeId::Integer), "integer");
  EXPECT_EQ(canonicalSqlTypeName(SqlTypeId::DoublePrecision),
            "double precision");
  EXPECT_EQ(canonicalSqlTypeName(SqlTypeId::TimestampWithTimeZone),
            "timestamp with time zone");
}

TEST(SqlTypeRegistryTest, ValidatesTypeModifiers) {
  const auto character = resolveSqlType("char");
  EXPECT_EQ(character.modifier.length, 1U);

  const auto varchar = resolveSqlType("varchar", {.length = 32});
  EXPECT_EQ(varchar.modifier.length, 32U);

  const auto numeric = resolveSqlType("decimal", {.precision = 18, .scale = 4});
  EXPECT_EQ(numeric.modifier.precision, 18U);
  EXPECT_EQ(numeric.modifier.scale, 4);

  const auto timestamp = resolveSqlType("timestamp", {.temporalPrecision = 3});
  EXPECT_EQ(timestamp.modifier.temporalPrecision, 3U);
}

TEST(SqlTypeRegistryTest, RejectsUnknownNamesAndInvalidModifiers) {
  EXPECT_THROW(resolveSqlType("money"), std::invalid_argument);
  EXPECT_THROW(resolveSqlType(""), std::invalid_argument);
  EXPECT_THROW(resolveSqlType("integer", {.length = 4}), std::invalid_argument);
  EXPECT_THROW(resolveSqlType("char", {.length = 0}), std::invalid_argument);
  EXPECT_THROW(resolveSqlType("varchar", {.length = 0}), std::invalid_argument);
  EXPECT_THROW(resolveSqlType("numeric", {.precision = 1001}),
               std::invalid_argument);
  EXPECT_THROW(resolveSqlType("numeric", {.scale = 2}), std::invalid_argument);
  EXPECT_THROW(resolveSqlType("numeric", {.precision = 10, .scale = 1001}),
               std::invalid_argument);
  EXPECT_THROW(resolveSqlType("timestamp", {.temporalPrecision = 7}),
               std::invalid_argument);
}
