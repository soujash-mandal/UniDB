#include "../../SqlRequestFramer.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(SqlRequestFramerTest, AcceptsCrLfLineEndings) {
  ConnectionProtocol::SqlRequestFramer framer;
  const auto requests = framer.append("SELECT * FROM people;\r\n");

  ASSERT_EQ(requests.size(), 1U);
  EXPECT_EQ(requests.front(), "SELECT * FROM people;");
}

TEST(SqlRequestFramerTest, RejectsEmptyRequest) {
  ConnectionProtocol::SqlRequestFramer framer;
  EXPECT_THROW(framer.append("\n"), std::invalid_argument);
}
