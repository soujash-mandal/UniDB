#include "../domain/codec/SqlTupleCodec.h"
#include "../domain/types/SqlNumericSemantics.h"
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
  EXPECT_EQ(bytes.size(), 24U);
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

TEST(SqlTupleCodecTest, RejectsTruncatedAndInvalidDeclaredRowHeaders) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"value", SqlType::Int, SqlTypeDescriptor{SqlTypeId::SmallInt, {}}}};
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{'B','A','D','!','\0','\0','\0','\0'}), std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{'U','D','B','R','\x02','\0','\x01','\0'}), std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{'U','D','B','R','\x01','\0','\x02','\0'}), std::runtime_error);
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
  ASSERT_EQ(bytes.size(), 11U);
  EXPECT_EQ(std::string(bytes.begin(), bytes.begin() + 4), "UDBR");
  EXPECT_EQ(static_cast<unsigned char>(bytes[8]), 0U);
  EXPECT_EQ(static_cast<unsigned char>(bytes[9]), 0U);
  EXPECT_EQ(static_cast<unsigned char>(bytes[10]), 1U);
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
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{'U','D','B','R','\x01','\0','\x01','\0'}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(schema, std::vector<char>{'U','D','B','R','\x01','\0','\x01','\0','\x02'}),
               std::runtime_error);
  EXPECT_THROW(SqlTupleCodec::decode(
                   schema, std::vector<char>{'U','D','B','R','\x01','\0','\x01','\0', static_cast<char>(0xff)}),
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
  EXPECT_EQ(bytes.size(), 21U);
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

TEST(SqlTupleCodecTest, RoundTripsNumericAlongsideOtherDeclaredTypes) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlNumericValue;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"id", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Integer, {}}},
      {"active", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Boolean, {}}},
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}},
      {"label", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}},
      {"ratio", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Real, {}}}};
  const std::vector<unidb::sql::SqlValue> values{
      std::int64_t{42}, true, SqlNumericValue{true, "1234567890123456789", 4},
      std::string{"invoice"}, 1.25F};

  const auto encoded = SqlTupleCodec::encode(schema, values);
  const auto decoded = SqlTupleCodec::decode(schema, encoded);

  EXPECT_EQ(decoded, values);
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(decoded[2])),
            "-123456789012345.6789");
}

TEST(SqlTupleCodecTest, RejectsNumericRuntimeTypeMismatch) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  EXPECT_THROW(SqlTupleCodec::encode(schema, {std::int64_t{123}}),
               std::invalid_argument);
}

TEST(SqlTupleCodecTest, RejectsMalformedNumericPayloadInsideDeclaredRow) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlNumericValue;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  auto encoded =
      SqlTupleCodec::encode(schema, {SqlNumericValue{false, "12345", 2}});
  ASSERT_GT(encoded.size(), 5U);
  encoded[13] = static_cast<char>(0x7f);
  EXPECT_THROW(SqlTupleCodec::decode(schema, encoded), std::runtime_error);

  encoded = SqlTupleCodec::encode(schema, {SqlNumericValue{false, "12345", 2}});
  encoded.pop_back();
  EXPECT_THROW(SqlTupleCodec::decode(schema, encoded), std::runtime_error);
}

TEST(SqlTupleCodecTest, RejectsTrailingBytesAfterNumericDeclaredRow) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlNumericValue;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  auto encoded =
      SqlTupleCodec::encode(schema, {SqlNumericValue{false, "123", 1}});
  encoded.push_back('\0');
  EXPECT_THROW(SqlTupleCodec::decode(schema, encoded), std::runtime_error);
}

TEST(SqlTupleCodecTest, RoundTripsNullValuesInDeclaredRows) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlNullValue;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;
  const std::vector<ColumnDefinition> schema{
      {"id", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Integer, {}}},
      {"label", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}},
      {"active", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Boolean, {}}}};
  const std::vector<unidb::sql::SqlValue> values{
      std::int64_t{7}, SqlNullValue{}, true};
  const auto bytes = SqlTupleCodec::encode(schema, values);
  ASSERT_EQ(std::string(bytes.begin(), bytes.begin() + 4), "UDBR");
  EXPECT_EQ(static_cast<unsigned char>(bytes[8]), 0x02U);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}

TEST(SqlTupleCodecTest, SupportsAllNullDeclaredRows) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlNullValue;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;
  const std::vector<ColumnDefinition> schema{
      {"first", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Integer, {}}},
      {"second", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}}};
  const std::vector<unidb::sql::SqlValue> values{SqlNullValue{}, SqlNullValue{}};
  const auto bytes = SqlTupleCodec::encode(schema, values);
  EXPECT_EQ(bytes.size(), 9U);
  EXPECT_EQ(static_cast<unsigned char>(bytes[8]), 0x03U);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}
