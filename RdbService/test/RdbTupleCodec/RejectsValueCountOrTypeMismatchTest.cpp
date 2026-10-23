#include "../../adapter/RdbTupleCodec.h"

#include <gtest/gtest.h>

TEST(RdbTupleCodecValidationTest, RejectsValueCountMismatch) {
  const std::vector<Column> schema{{"id", DataType::INT, 0, false}};
  EXPECT_THROW(unidb::rdb::RdbTupleCodec::encode(schema, {}),
               std::invalid_argument);
}

TEST(RdbTupleCodecValidationTest, RejectsValueTypeMismatch) {
  const std::vector<Column> schema{{"id", DataType::INT, 0, false}};
  const std::vector<unidb::sql::SqlValue> values{std::string{"not-an-int"}};
  EXPECT_THROW(unidb::rdb::RdbTupleCodec::encode(schema, values),
               std::invalid_argument);
}
