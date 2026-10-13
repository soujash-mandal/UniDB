#include "../adapter/SqlTupleCodec.h"
#include <gtest/gtest.h>

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
