#include "../domain/codec/SqlTupleCodec.h"
#include <gtest/gtest.h>

#include <cmath>
#include <limits>

TEST(SqlTupleCodecTest, RoundTripsIntAndText) {
  std::vector<unidb::sql::SqlType> schema;
  schema.push_back(unidb::sql::SqlType::Int);
  schema.push_back(unidb::sql::SqlType::Text);
  const std::vector<unidb::sql::SqlValue> values{int64_t{42},
                                                 std::string("hello")};
  const auto bytes = SqlTupleCodec::encode(schema, values);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}

TEST(SqlTupleCodecTest, RejectsTypeMismatchAndWrongArity) {
  const std::vector<unidb::sql::SqlType> schema{unidb::sql::SqlType::Int};
  EXPECT_THROW(SqlTupleCodec::encode(schema, {std::string("not int")}),
               std::invalid_argument);
  EXPECT_THROW(SqlTupleCodec::encode(schema, {}), std::invalid_argument);
}

TEST(SqlTupleCodecTest, RejectsTruncatedFieldsAndTrailingBytes) {
  const std::vector<unidb::sql::SqlType> intSchema{unidb::sql::SqlType::Int};
  const std::vector<unidb::sql::SqlType> textSchema{unidb::sql::SqlType::Text};
  EXPECT_THROW(SqlTupleCodec::decode(intSchema, std::vector<char>{'x'}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(textSchema, std::vector<char>{1, 0, 0}),
               std::runtime_error);

  auto encoded = SqlTupleCodec::encode(intSchema, {int64_t{42}});
  encoded.push_back('x');
  EXPECT_THROW(SqlTupleCodec::decode(intSchema, encoded), std::runtime_error);
}

TEST(SqlTupleCodecTest, RoundTripsPostgreSqlIntegerWidthsUsingDeclaredSchema) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"small_value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::SmallInt, {}}},
      {"integer_value", SqlType::Int,
       SqlTypeDescriptor{SqlTypeId::Integer, {}}},
      {"big_value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::BigInt, {}}}};
  const std::vector<unidb::sql::SqlValue> values{
      std::int64_t{-32768}, std::int64_t{2147483647},
      std::numeric_limits<std::int64_t>::min()};

  const auto bytes = SqlTupleCodec::encode(schema, values);
  EXPECT_EQ(bytes.size(), 15U);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}

TEST(SqlTupleCodecTest, RejectsIntegerOverflowForDeclaredWidth) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> smallSchema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::SmallInt, {}}}};
  const std::vector<ColumnDefinition> integerSchema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Integer, {}}}};

  EXPECT_THROW(SqlTupleCodec::encode(smallSchema, {std::int64_t{32768}}),
               std::out_of_range);
  EXPECT_THROW(
      SqlTupleCodec::encode(integerSchema, {std::int64_t{2147483648LL}}),
      std::out_of_range);
}

TEST(SqlTupleCodecTest, RejectsTruncatedAndUnknownVersionDeclaredRows) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::SmallInt, {}}}};
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{0x01, 0x01}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{0x02}),
               std::runtime_error);
  EXPECT_THROW(
      SqlTupleCodec::decode(schema, std::vector<char>{0x01, 0x01, 0x00, 0x02}),
      std::runtime_error);
}

TEST(SqlTupleCodecTest, RoundTripsBooleanValuesUsingDeclaredSchema) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"false_value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Boolean, {}}},
      {"true_value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Boolean, {}}}};
  const std::vector<unidb::sql::SqlValue> values{false, true};

  const auto bytes = SqlTupleCodec::encode(schema, values);
  ASSERT_EQ(bytes.size(), 3U);
  EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 1U);
  EXPECT_EQ(static_cast<unsigned char>(bytes[1]), 0U);
  EXPECT_EQ(static_cast<unsigned char>(bytes[2]), 1U);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}

TEST(SqlTupleCodecTest, RejectsNonBooleanValuesForDeclaredBooleanColumns) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Boolean, {}}}};
  EXPECT_THROW(SqlTupleCodec::encode(schema, {std::int64_t{1}}),
               std::invalid_argument);
  EXPECT_THROW(SqlTupleCodec::encode(schema, {std::string("true")}),
               std::invalid_argument);
}

TEST(SqlTupleCodecTest, RejectsInvalidBooleanByteAndTruncatedBooleanRows) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Boolean, {}}}};
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{0x01}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{0x01, 0x02}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(
                   schema, std::vector<char>{0x01, static_cast<char>(0xff)}),
               std::runtime_error);
}

TEST(SqlTupleCodecTest, RoundTripsRealAndDoublePrecisionUsingDeclaredSchema) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"real_value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Real, {}}},
      {"double_value", SqlType::Int,
       SqlTypeDescriptor{SqlTypeId::DoublePrecision, {}}}};
  const std::vector<unidb::sql::SqlValue> values{1.25F, -123.5};

  const auto bytes = SqlTupleCodec::encode(schema, values);
  EXPECT_EQ(bytes.size(), 13U);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}

TEST(SqlTupleCodecTest, PreservesFloatingPointSpecialValuesAndSignedZero) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"real_value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Real, {}}},
      {"double_value", SqlType::Int,
       SqlTypeDescriptor{SqlTypeId::DoublePrecision, {}}}};
  const std::vector<unidb::sql::SqlValue> values{
      -0.0F, std::numeric_limits<double>::infinity()};

  const auto bytes = SqlTupleCodec::encode(schema, values);
  const auto decoded = SqlTupleCodec::decode(schema, bytes);
  ASSERT_EQ(decoded.size(), 2U);
  EXPECT_TRUE(std::signbit(std::get<float>(decoded[0])));
  EXPECT_EQ(std::get<double>(decoded[1]),
            std::numeric_limits<double>::infinity());
}

TEST(SqlTupleCodecTest, RejectsFloatingPointRuntimeWidthMismatch) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> realSchema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Real, {}}}};
  const std::vector<ColumnDefinition> doubleSchema{
      {"value", SqlType::Int,
       SqlTypeDescriptor{SqlTypeId::DoublePrecision, {}}}};

  EXPECT_THROW(SqlTupleCodec::encode(realSchema, {1.0}), std::invalid_argument);
  EXPECT_THROW(SqlTupleCodec::encode(doubleSchema, {1.0F}),
               std::invalid_argument);
  EXPECT_THROW(SqlTupleCodec::encode(realSchema, {std::int64_t{1}}),
               std::invalid_argument);
}

TEST(SqlTupleCodecTest, RejectsTruncatedAndTrailingFloatingPointBytes) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Real, {}}}};
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{0x01, 0x00}),
               std::runtime_error);

  auto bytes = SqlTupleCodec::encode(schema, {1.5F});
  bytes.push_back(0x00);
  EXPECT_THROW(SqlTupleCodec::decode(schema, bytes), std::runtime_error);
}
