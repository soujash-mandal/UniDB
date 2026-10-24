#include "../../domain/RdbRowCodec.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(RdbRowCodecTest, RejectsValueCountThatDoesNotMatchSchema) {
  RdbTableDefinition table;
  table.columns = {{"id", RdbDataType::INT, 4, false}};

  EXPECT_THROW(RdbRowCodec::encode(table, {}), std::invalid_argument);
}
