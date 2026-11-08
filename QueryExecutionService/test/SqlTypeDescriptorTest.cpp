#include "../type/type.h"

#include <gtest/gtest.h>

TEST(SqlTypeDescriptorTest, DefinesCanonicalIdentityForEveryInitialTypeFamily) {
  using unidb::sql::SqlTypeId;

  EXPECT_NE(SqlTypeId::SmallInt, SqlTypeId::Integer);
  EXPECT_NE(SqlTypeId::Integer, SqlTypeId::BigInt);
  EXPECT_NE(SqlTypeId::Real, SqlTypeId::DoublePrecision);
  EXPECT_NE(SqlTypeId::Numeric, SqlTypeId::Real);
  EXPECT_NE(SqlTypeId::Boolean, SqlTypeId::Integer);
  EXPECT_NE(SqlTypeId::Char, SqlTypeId::Varchar);
  EXPECT_NE(SqlTypeId::Varchar, SqlTypeId::Text);
  EXPECT_NE(SqlTypeId::Timestamp, SqlTypeId::TimestampWithTimeZone);
  EXPECT_NE(SqlTypeId::Json, SqlTypeId::Jsonb);
}

TEST(SqlTypeDescriptorTest, StoresTypeSpecificModifiersWithoutEncodingPolicy) {
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const SqlTypeDescriptor varchar{SqlTypeId::Varchar, {.length = 64}};
  const SqlTypeDescriptor numeric{SqlTypeId::Numeric,
                                  {.precision = 12, .scale = 3}};
  const SqlTypeDescriptor timestamp{SqlTypeId::Timestamp,
                                    {.temporalPrecision = 6}};

  EXPECT_EQ(varchar.modifier.length, 64U);
  EXPECT_EQ(numeric.modifier.precision, 12U);
  EXPECT_EQ(numeric.modifier.scale, 3);
  EXPECT_EQ(timestamp.modifier.temporalPrecision, 6U);
}

TEST(SqlTypeDescriptorTest, SupportsValueEquality) {
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const SqlTypeDescriptor first{SqlTypeId::Numeric,
                                {.precision = 10, .scale = 2}};
  const SqlTypeDescriptor same{SqlTypeId::Numeric,
                               {.precision = 10, .scale = 2}};
  const SqlTypeDescriptor different{SqlTypeId::Numeric,
                                    {.precision = 10, .scale = 3}};

  EXPECT_EQ(first, same);
  EXPECT_NE(first, different);
}

TEST(SqlTypeDescriptorTest, KeepsExistingExecutionTypesAvailable) {
  using unidb::sql::SqlType;

  EXPECT_NE(SqlType::Int, SqlType::Text);
}
