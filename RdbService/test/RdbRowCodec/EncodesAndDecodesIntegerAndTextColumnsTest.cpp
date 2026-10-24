#include "../../domain/RdbRowCodec.h"

#include <gtest/gtest.h>

TEST(RdbRowCodecTest, EncodesAndDecodesIntegerAndTextColumns) {
  RdbTableDefinition table;
  table.name = "users";
  table.columns = {
      {"id", RdbDataType::INT, 4, false},
      {"name", RdbDataType::VARCHAR, 32, false},
  };
  const RdbRow row = {int64_t{17}, std::string("Ada")};

  const auto encoded = RdbRowCodec::encode(table, row);

  EXPECT_EQ(RdbRowCodec::decode(table, encoded), row);
}
