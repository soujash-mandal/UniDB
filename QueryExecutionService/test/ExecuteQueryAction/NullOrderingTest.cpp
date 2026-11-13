#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <variant>

namespace {

FakeQesRdbServicePort makeNullableOrderPort() {
  FakeQesRdbServicePort port;
  port.schema = {
      {"id", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Integer, {}}},
      {"label", unidb::sql::SqlType::Text,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Text, {}}}};
  port.schema[1].nullable = true;
  port.rows = {
      SqlTupleCodec::encode(port.schema,
                            {std::int64_t{1}, unidb::sql::SqlNullValue{}}),
      SqlTupleCodec::encode(port.schema,
                            {std::int64_t{2}, std::string("b")}),
      SqlTupleCodec::encode(port.schema,
                            {std::int64_t{3}, std::string("a")})};
  return port;
}

const QueryExecutionService::QueryExecutionResult &
expectRows(const QueryExecutionService::QueryExecutionOutcome &outcome) {
  EXPECT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  return std::get<QueryExecutionService::QueryExecutionResult>(outcome);
}

} // namespace

TEST(ExecuteQueryActionNullOrderingTest, AscendingPlacesNullsLast) {
  auto port = makeNullableOrderPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT label, id FROM people ORDER BY label ASC;");
  const auto &result = expectRows(outcome);

  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "a");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "b");
  EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(
      result.rows[2][0]));
}

TEST(ExecuteQueryActionNullOrderingTest, DescendingPlacesNullsFirst) {
  auto port = makeNullableOrderPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT label, id FROM people ORDER BY label DESC;");
  const auto &result = expectRows(outcome);

  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(
      result.rows[0][0]));
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "b");
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "a");
}

TEST(ExecuteQueryActionNullOrderingTest, AppliesDefaultsToGroupedOrdering) {
  auto port = makeNullableOrderPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT label, COUNT(*) FROM people "
                     "GROUP BY label ORDER BY label ASC;");
  const auto &result = expectRows(outcome);

  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "a");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "b");
  EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(
      result.rows[2][0]));
}
