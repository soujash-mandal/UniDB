#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <limits>
#include <vector>

namespace {
FakeQesRdbServicePort makeFloatingPointPort() {
  FakeQesRdbServicePort port;
  port.schema = {
      {"approximate", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Real, {}}},
      {"precise", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{
           unidb::sql::SqlTypeId::DoublePrecision, {}}}};
  const float realNaN = std::numeric_limits<float>::quiet_NaN();
  const double doubleNaN = std::numeric_limits<double>::quiet_NaN();
  port.rows = {
      SqlTupleCodec::encode(port.schema, {1.0F, 10.0}),
      SqlTupleCodec::encode(port.schema, {1.0F, 10.0}),
      SqlTupleCodec::encode(port.schema, {-0.0F, -0.0}),
      SqlTupleCodec::encode(port.schema, {0.0F, 0.0}),
      SqlTupleCodec::encode(port.schema, {realNaN, doubleNaN}),
      SqlTupleCodec::encode(port.schema, {realNaN, doubleNaN})};
  return port;
}
} // namespace

TEST(ExecuteQueryActionFloatingPointTest,
     DistinctUsesSqlEqualityForRealAndDoublePrecision) {
  auto port = makeFloatingPointPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto realOutcome = action.execute(
      "SELECT DISTINCT approximate FROM measurements ORDER BY approximate;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(realOutcome));
  const auto &realRows =
      std::get<QueryExecutionService::QueryExecutionResult>(realOutcome).rows;
  ASSERT_EQ(realRows.size(), 3U);
  EXPECT_FLOAT_EQ(std::get<float>(realRows[0][0]), 0.0F);
  EXPECT_FLOAT_EQ(std::get<float>(realRows[1][0]), 1.0F);
  EXPECT_TRUE(std::isnan(std::get<float>(realRows[2][0])));

  const auto doubleOutcome = action.execute(
      "SELECT DISTINCT precise FROM measurements ORDER BY precise;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(doubleOutcome));
  const auto &doubleRows =
      std::get<QueryExecutionService::QueryExecutionResult>(doubleOutcome).rows;
  ASSERT_EQ(doubleRows.size(), 3U);
  EXPECT_DOUBLE_EQ(std::get<double>(doubleRows[0][0]), 0.0);
  EXPECT_DOUBLE_EQ(std::get<double>(doubleRows[1][0]), 10.0);
  EXPECT_TRUE(std::isnan(std::get<double>(doubleRows[2][0])));
}

TEST(ExecuteQueryActionFloatingPointTest,
     GroupsFloatingPointNaNAndSignedZeroUsingSqlEquality) {
  auto port = makeFloatingPointPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT approximate, COUNT(*) FROM measurements "
      "GROUP BY approximate ORDER BY approximate;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_FLOAT_EQ(std::get<float>(rows[0][0]), 0.0F);
  EXPECT_EQ(std::get<std::int64_t>(rows[0][1]), 2);
  EXPECT_FLOAT_EQ(std::get<float>(rows[1][0]), 1.0F);
  EXPECT_EQ(std::get<std::int64_t>(rows[1][1]), 2);
  EXPECT_TRUE(std::isnan(std::get<float>(rows[2][0])));
  EXPECT_EQ(std::get<std::int64_t>(rows[2][1]), 2);
}
