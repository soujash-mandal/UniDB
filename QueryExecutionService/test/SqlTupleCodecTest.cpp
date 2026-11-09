#include "../domain/codec/SqlTupleCodec.h"
#include <gtest/gtest.h>

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
