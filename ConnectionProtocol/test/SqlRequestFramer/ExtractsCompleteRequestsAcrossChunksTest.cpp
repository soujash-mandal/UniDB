#include "../../SqlRequestFramer.h"

#include <gtest/gtest.h>

TEST(SqlRequestFramerTest, ExtractsCompleteRequestsAcrossChunks) {
  ConnectionProtocol::SqlRequestFramer framer;

  EXPECT_TRUE(framer.append("SELECT * FR").requests.empty());
  EXPECT_TRUE(framer.hasPendingBytes());

  const auto result =
      framer.append("OM people;\nINSERT INTO people VALUES (1);\n");
  ASSERT_FALSE(result.error.has_value());
  ASSERT_EQ(result.requests.size(), 2U);
  EXPECT_EQ(result.requests[0], "SELECT * FROM people;");
  EXPECT_EQ(result.requests[1], "INSERT INTO people VALUES (1);");
  EXPECT_FALSE(framer.hasPendingBytes());
}
