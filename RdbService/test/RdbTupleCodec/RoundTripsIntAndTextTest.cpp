#include "../../adapter/RdbTupleCodec.h"

#include <gtest/gtest.h>

TEST(RdbTupleCodecRoundTripTest, RoundTripsIntAndText) {
  const std::vector<Column> schema{{"id", DataType::INT, 0, false},
                                   {"name", DataType::VARCHAR, 0, false}};
  const std::vector<unidb::sql::SqlValue> input{std::int64_t{-42},
                                                std::string{"Ada Lovelace"}};

  const auto bytes = unidb::rdb::RdbTupleCodec::encode(schema, input);

  EXPECT_EQ(unidb::rdb::RdbTupleCodec::decode(schema, bytes), input);
}
