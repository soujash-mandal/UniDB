#include "../adapter/SqlTupleCodec.h"
#include "../../TupleService/domain/TuplePage.h"
#include <gtest/gtest.h>

TEST(SqlTupleCodecTest, RoundTripsIntAndText) {
  const std::vector<Column> schema{{"id", DataType::INT},
                                   {"name", DataType::VARCHAR}};
  const std::vector<unidb::sql::SqlValue> values{int64_t{42},
                                                 std::string("hello")};
  const auto bytes = SqlTupleCodec::encode(schema, values);
  EXPECT_EQ(SqlTupleCodec::decode(schema, bytes), values);
}
TEST(SqlTupleCodecTest, RejectsTypeMismatchAndWrongArity) {
  const std::vector<Column> schema{{"id", DataType::INT}};
  EXPECT_THROW(SqlTupleCodec::encode(schema, {std::string("not int")}),
               std::invalid_argument);
  EXPECT_THROW(SqlTupleCodec::encode(schema, {}), std::invalid_argument);
}
TEST(TuplePageTest, MaintainsPageChainAndSkipsDeletedSlots) {
  TuplePage page;
  const auto first = page.insert({'a', 'b'});
  page.insert({'c'});
  page.remove(first);
  EXPECT_EQ(page.getAllTuples().size(), 1U);
  EXPECT_EQ(page.getAllTuples().front(), (std::vector<char>{'c'}));
  page.setNextPageId(17);
  EXPECT_EQ(page.getNextPageId(), 17U);
}
