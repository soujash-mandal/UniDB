#include "../domain/codec/SqlTupleCodec.h"
#include "../type/type.h"

#include <gtest/gtest.h>

TEST(SqlTupleCodec, RoundTripsDeclaredCharacterTypes) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;
  using unidb::sql::SqlTypeModifier;

  SqlTypeModifier fixedLength;
  fixedLength.length = 4;
  SqlTypeModifier boundedLength;
  boundedLength.length = 12;
  const std::vector<ColumnDefinition> schema{
      ColumnDefinition{"fixed", SqlType::Text,
                       SqlTypeDescriptor{SqlTypeId::Char, fixedLength}},
      ColumnDefinition{"bounded", SqlType::Text,
                       SqlTypeDescriptor{SqlTypeId::Varchar, boundedLength}},
      ColumnDefinition{"unbounded", SqlType::Text,
                       SqlTypeDescriptor{SqlTypeId::Varchar, {}}},
      ColumnDefinition{"body", SqlType::Text,
                       SqlTypeDescriptor{SqlTypeId::Text, {}}},
  };
  const std::vector<unidb::sql::SqlValue> values{
      std::string("ab  "), std::string("hello"), std::string("world"),
      std::string("café 東京")};

  const auto bytes = SqlTupleCodec::encode(schema, values);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}

TEST(SqlTupleCodec, RejectsMalformedUtf8InCharacterRows) {
  using unidb::sql::ColumnDefinition;
  using unidb::sql::SqlType;
  using unidb::sql::SqlTypeDescriptor;
  using unidb::sql::SqlTypeId;

  const std::vector<ColumnDefinition> schema{
      ColumnDefinition{"body", SqlType::Text,
                       SqlTypeDescriptor{SqlTypeId::Text, {}}},
  };
  const std::vector<unidb::sql::SqlValue> values{std::string("\xc0", 1)};
  EXPECT_THROW(SqlTupleCodec::encode(schema, values), std::invalid_argument);

  const std::vector<char> malformedRow{'U', 'D', 'B', 'R', 1, 0, 1, 0, 0,\n                                  1, 0, 0, 0, static_cast<char>(0xc0)};
  EXPECT_THROW(SqlTupleCodec::decode(schema, malformedRow),
               std::invalid_argument);
}
