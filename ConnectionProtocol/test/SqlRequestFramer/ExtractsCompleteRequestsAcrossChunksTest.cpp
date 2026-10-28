#include "../../SqlRequestFramer.h"

#include <gtest/gtest.h>

TEST(SqlRequestFramerTest, ExtractsCompleteRequestsAcrossChunks) {
  ConnectionProtocol::SqlRequestFramer framer;

  EXPECT_TRUE(framer.append("SELECT * FR").empty());
  EXPECT_TRUE(framer.hasPendingBytes());

  const auto requests =
      framer.append("OM people;\nINSERT INTO people VALUES (1);\n");
  ASSERT_EQ(requests.size(), 2U);
  EXPECT_EQ(requests[0], "SELECT * FROM people;");
  EXPECT_EQ(requests[1], "INSERT INTO people VALUES (1);");
  EXPECT_FALSE(framer.hasPendingBytes());
}
