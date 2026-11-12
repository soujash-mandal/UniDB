#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::SqlType;
using unidb::sql::SqlTypeDescriptor;
using unidb::sql::SqlTypeId;

ColumnDefinition nullableColumn(const std::string &name, SqlType type,
                                SqlTypeId declaredType) {
  ColumnDefinition column;
  column.name = name;
  column.type = type;
  column.declaredType = SqlTypeDescriptor{declaredType, {}};
  column.nullable = true;
  return column;
}

FakeQesRdbServicePort makeNullableAggregatePort() {
  FakeQesRdbServicePort port;
  port.schema = {nullableColumn("department", SqlType::Text, SqlTypeId::Text),
                 nullableColumn("amount", SqlType::Int, SqlTypeId::Integer)};
  port.rows = {
      SqlTupleCodec::encode(port.schema,
                            {std::string("eng"), std::int64_t{10}}),
      SqlTupleCodec::encode(port.schema,
                            {std::string("eng"), unidb::sql::SqlNullValue{}}),
      SqlTupleCodec::encode(port.schema,
                            {std::string("sales"), std::int64_t{20}}),
      SqlTupleCodec::encode(
          port.schema, {std::string("sales"), unidb::sql::SqlNullValue{}})};
  return port;
}

template <typename Outcome>
const QueryExecutionService::QueryExecutionResult &
expectResult(const Outcome &outcome) {
  EXPECT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  return std::get<QueryExecutionService::QueryExecutionResult>(outcome);
}

} // namespace

TEST(ExecuteQueryActionNullAggregateTest,
     DistinguishesCountStarFromCountColumnAndSkipsNullInputs) {
  auto port = makeNullableAggregatePort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto countStar = action.execute("SELECT COUNT(*) FROM people;");
  ASSERT_EQ(expectResult(countStar).rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(expectResult(countStar).rows[0][0]), 4);

  const auto countColumn = action.execute("SELECT COUNT(amount) FROM people;");
  EXPECT_EQ(std::get<std::int64_t>(expectResult(countColumn).rows[0][0]), 2);

  const auto sum = action.execute("SELECT SUM(amount) FROM people;");
  EXPECT_EQ(std::get<std::int64_t>(expectResult(sum).rows[0][0]), 30);

  const auto average = action.execute("SELECT AVG(amount) FROM people;");
  EXPECT_EQ(std::get<std::int64_t>(expectResult(average).rows[0][0]), 15);

  const auto minimum = action.execute("SELECT MIN(amount) FROM people;");
  EXPECT_EQ(std::get<std::int64_t>(expectResult(minimum).rows[0][0]), 10);

  const auto maximum = action.execute("SELECT MAX(amount) FROM people;");
  EXPECT_EQ(std::get<std::int64_t>(expectResult(maximum).rows[0][0]), 20);
}

TEST(ExecuteQueryActionNullAggregateTest, ReturnsNullWhenAllValuesAreNull) {
  auto port = makeNullableAggregatePort();
  port.rows.clear();
  port.rows = {
      SqlTupleCodec::encode(port.schema,
                            {std::string("eng"), unidb::sql::SqlNullValue{}}),
      SqlTupleCodec::encode(
          port.schema, {std::string("sales"), unidb::sql::SqlNullValue{}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto count = action.execute("SELECT COUNT(amount) FROM people;");
  EXPECT_EQ(std::get<std::int64_t>(expectResult(count).rows[0][0]), 0);

  for (const auto *sql :
       {"SELECT SUM(amount) FROM people;", "SELECT AVG(amount) FROM people;",
        "SELECT MIN(amount) FROM people;", "SELECT MAX(amount) FROM people;"}) {
    const auto outcome = action.execute(sql);
    ASSERT_EQ(expectResult(outcome).rows.size(), 1U) << sql;
    EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(
        expectResult(outcome).rows[0][0]))
        << sql;
  }
}

TEST(ExecuteQueryActionNullAggregateTest,
     GroupedAggregatesReturnNullAndHavingFiltersUnknown) {
  auto port = makeNullableAggregatePort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  port.rows = {
      SqlTupleCodec::encode(port.schema,
                            {std::string("eng"), unidb::sql::SqlNullValue{}}),
      SqlTupleCodec::encode(
          port.schema, {std::string("sales"), unidb::sql::SqlNullValue{}})};
  const auto grouped =
      action.execute("SELECT department, SUM(amount) FROM people "
                     "GROUP BY department ORDER BY department ASC;");
  ASSERT_EQ(expectResult(grouped).rows.size(), 2U);
  EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(
      expectResult(grouped).rows[0][1]));
  EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(
      expectResult(grouped).rows[1][1]));

  port.rows = {SqlTupleCodec::encode(port.schema, {std::string("eng"),
                                                   unidb::sql::SqlNullValue{}}),
               SqlTupleCodec::encode(port.schema,
                                     {std::string("sales"), std::int64_t{20}})};
  const auto having =
      action.execute("SELECT department, SUM(amount) FROM people "
                     "GROUP BY department HAVING SUM(amount) > 0;");
  ASSERT_EQ(expectResult(having).rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(expectResult(having).rows[0][0]), "sales");
}
