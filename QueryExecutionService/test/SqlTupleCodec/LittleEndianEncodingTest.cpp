#include "../adapter/SqlTupleCodec.h"

#include <gtest/gtest.h>

TEST(SqlTupleCodecLittleEndianEncodingTest,
     EncodesIntAndTextLengthLittleEndian) {
  const std::vector<unidb::sql::SqlType> schema{unidb::sql::SqlType::Int,
                                                unidb::sql::SqlType::Text};
  const std::vector<unidb::sql::SqlValue> values{int64_t{0x0102030405060708LL},
                                                 std::string("abc")};

  const auto bytes = SqlTupleCodec::encode(schema, values);
  const std::vector<char> expected{0x08, 0x07, 0x06, 0x05, 0x04,
                                   0x03, 0x02, 0x01, 0x03, 0x00,
                                   0x00, 0x00, 'a',  'b',  'c'};
  EXPECT_EQ(bytes, expected);
}
