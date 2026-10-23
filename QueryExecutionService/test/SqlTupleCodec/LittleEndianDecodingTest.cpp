#include "../adapter/SqlTupleCodec.h"

#include <gtest/gtest.h>

TEST(SqlTupleCodecLittleEndianDecodingTest, DecodesKnownLittleEndianBytes) {
  const std::vector<unidb::sql::SqlType> schema{
      unidb::sql::SqlType::Int, unidb::sql::SqlType::Text};
  const std::vector<char> bytes{
      0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
      0x03, 0x00, 0x00, 0x00, 'a', 'b', 'c'};
  const std::vector<unidb::sql::SqlValue> expected{
      int64_t{0x0102030405060708LL}, std::string("abc")};

  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), expected);
}
