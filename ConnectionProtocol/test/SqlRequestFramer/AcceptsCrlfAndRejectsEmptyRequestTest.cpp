#include "../../SqlRequestFramer.h"

#include <gtest/gtest.h>

#include <string>

TEST(SqlRequestFramerTest, AcceptsCrLfLineEndings) {
  ConnectionProtocol::SqlRequestFramer framer;
  const auto result = framer.append("SELECT * FROM people;\r\n");

  ASSERT_FALSE(result.error.has_value());
  ASSERT_EQ(result.requests.size(), 1U);
  EXPECT_EQ(result.requests.front(), "SELECT * FROM people;");
}

TEST(SqlRequestFramerTest, ReturnsEarlierRequestsAlongsideEmptyRequestError) {
  ConnectionProtocol::SqlRequestFramer framer;
  const auto result = framer.append("SELECT 1;\n\nSELECT 2;\n");

  ASSERT_EQ(result.requests.size(), 1U);
  EXPECT_EQ(result.requests.front(), "SELECT 1;");
  ASSERT_TRUE(result.error.has_value());
  EXPECT_EQ(*result.error, "SQL request must not be empty");
}

TEST(SqlRequestFramerTest, AcceptsMaximumLengthRequestWithCrLf) {
  ConnectionProtocol::SqlRequestFramer framer;
  const std::string sql(ConnectionProtocol::MAX_REQUEST_BYTES, 'x');
  const auto result = framer.append(sql + "\r\n");

  ASSERT_FALSE(result.error.has_value());
  ASSERT_EQ(result.requests.size(), 1U);
  EXPECT_EQ(result.requests.front(), sql);
}

TEST(SqlRequestFramerTest, AcceptsCrLfSplitAcrossReadsAtMaximumLength) {
  ConnectionProtocol::SqlRequestFramer framer;
  const std::string sql(ConnectionProtocol::MAX_REQUEST_BYTES, 'x');
  EXPECT_TRUE(framer.append(sql + "\r").requests.empty());

  const auto result = framer.append("\n");
  ASSERT_FALSE(result.error.has_value());
  ASSERT_EQ(result.requests.size(), 1U);
  EXPECT_EQ(result.requests.front(), sql);
}
