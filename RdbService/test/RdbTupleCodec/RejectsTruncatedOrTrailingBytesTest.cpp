#include "../../adapter/RdbTupleCodec.h"

#include <gtest/gtest.h>

TEST(RdbTupleCodecDecodeTest, RejectsTruncatedInt) {
  const std::vector<Column> schema{{"id", DataType::INT, 0, false}};
  EXPECT_THROW(
      unidb::rdb::RdbTupleCodec::decode(schema, std::vector<char>{1, 2}),
      std::runtime_error);
}

TEST(RdbTupleCodecDecodeTest, RejectsTrailingBytes) {
  const std::vector<Column> schema{{"id", DataType::INT, 0, false}};
  const std::vector<unidb::sql::SqlValue> values{std::int64_t{5}};
  auto bytes = unidb::rdb::RdbTupleCodec::encode(schema, values);
  bytes.push_back('x');
  EXPECT_THROW(unidb::rdb::RdbTupleCodec::decode(schema, bytes),
               std::runtime_error);
}
