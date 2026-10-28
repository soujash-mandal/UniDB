#include "../../SqlRequestFramer.h"

#include <gtest/gtest.h>

#include <string>

TEST(SqlRequestFramerTest, RejectsOversizedRequestEvenWhenSplitAcrossReads) {
  ConnectionProtocol::SqlRequestFramer framer;
  const std::string first(ConnectionProtocol::MAX_REQUEST_BYTES, 'x');
  EXPECT_TRUE(framer.append(first).requests.empty());

  const auto result = framer.append("x\n");
  EXPECT_TRUE(result.requests.empty());
  ASSERT_TRUE(result.error.has_value());
  EXPECT_EQ(*result.error, "SQL request exceeds 64 KiB limit");
}

TEST(SqlRequestFramerTest, PreservesEarlierRequestsBeforeOversizedRequest) {
  ConnectionProtocol::SqlRequestFramer framer;
  const std::string oversized(ConnectionProtocol::MAX_REQUEST_BYTES + 1, 'x');
  const auto result = framer.append("SELECT 1;\n" + oversized + "\n");

  ASSERT_EQ(result.requests.size(), 1U);
  EXPECT_EQ(result.requests.front(), "SELECT 1;");
  ASSERT_TRUE(result.error.has_value());
  EXPECT_EQ(*result.error, "SQL request exceeds 64 KiB limit");
}
