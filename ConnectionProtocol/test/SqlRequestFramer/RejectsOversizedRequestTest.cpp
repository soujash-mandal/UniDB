#include "../../SqlRequestFramer.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

TEST(SqlRequestFramerTest, RejectsOversizedRequestEvenWhenSplitAcrossReads) {
  ConnectionProtocol::SqlRequestFramer framer;
  const std::string first(ConnectionProtocol::MAX_REQUEST_BYTES, 'x');
  EXPECT_TRUE(framer.append(first).empty());
  EXPECT_THROW(framer.append("x\n"), std::length_error);
}
