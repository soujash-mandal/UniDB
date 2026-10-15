#include "serviceGraph.h"
#include <gtest/gtest.h>
#include <algorithm>

TEST(ServiceGraphTest, QueryExecutionDependsOnRdbAndRdbDoesNotDependOnFsm) {
  const auto hasEdge = [](const Architecture::ServiceEdge &expected) {
    return std::find(Architecture::SERVICE_EDGES.begin(),
                     Architecture::SERVICE_EDGES.end(), expected) !=
           Architecture::SERVICE_EDGES.end();
  };
  EXPECT_TRUE(hasEdge({"QueryExecutionService", "RdbService"}));
  EXPECT_FALSE(hasEdge({"RdbService", "QueryExecutionService"}));
  EXPECT_FALSE(hasEdge({"RdbService", "FreeSpaceMapService"}));
}
