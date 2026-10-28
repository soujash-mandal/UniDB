#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, InsertsValuesInSchemaOrder) {
  // Scenario 1: INSERT columns are reordered to match the stored table schema.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("INSERT INTO people (name, id) VALUES ('Ada', 7);");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto decoded = SqlTupleCodec::decode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      port.insertedBytes);
  ASSERT_EQ(decoded.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(decoded[0]), 7);
  EXPECT_EQ(std::get<std::string>(decoded[1]), "Ada");
  EXPECT_EQ(port.insertCalls, 1);
}
