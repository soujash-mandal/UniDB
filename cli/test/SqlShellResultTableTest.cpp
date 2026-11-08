#include "../../QueryExecutionService/domain/codec/SqlTupleCodec.h"
#include "../../QueryExecutionService/test/ExecuteQueryAction/ExecuteQueryActionTestSupport.h"
#include "../SqlShell.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

TEST(SqlShellTest, PrintsSelectResultsAsAlignedTable) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("Ada")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{12}, std::string("Grace Hopper")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("SELECT * FROM people;\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("+----+--------------+"), std::string::npos);
  EXPECT_NE(output.str().find("| id | name         |"), std::string::npos);
  EXPECT_NE(output.str().find("|  7 | Ada          |"), std::string::npos);
  EXPECT_NE(output.str().find("| 12 | Grace Hopper |"), std::string::npos);
  EXPECT_NE(output.str().find("2 row(s)"), std::string::npos);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, EscapesControlCharactersAndAlignsUtf8Text) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("A\tB")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{8}, std::string("Renée")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("SELECT * FROM people;\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("+----+-------+"), std::string::npos);
  EXPECT_NE(output.str().find("| id | name  |"), std::string::npos);
  EXPECT_NE(output.str().find("|  7 | A\\tB  |"), std::string::npos);
  EXPECT_NE(output.str().find("|  8 | Renée |"), std::string::npos);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, EscapesInvalidUtf8AndAlignsJoinedEmoji) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{9},
       std::string("bad ") + std::string(1, static_cast<char>(0xff))}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{10}, std::string("👩‍💻")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{11}, std::string("C1 ") + std::string("\xC2\x9B", 2)}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{12}, std::string("a | b")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{13}, std::string("\\t")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("SELECT * FROM people;\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("bad \\xFF"), std::string::npos);
  EXPECT_NE(output.str().find("| 10 | 👩‍💻        |"),
            std::string::npos);
  EXPECT_NE(output.str().find("C1 \\u009B"), std::string::npos);
  EXPECT_NE(output.str().find("a \\| b"), std::string::npos);
  EXPECT_NE(output.str().find("\\\\t"), std::string::npos);
  EXPECT_NE(output.str().find("+----+-----------+"), std::string::npos);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, PrintsHeadersAndBordersForEmptyResultSet) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("SELECT * FROM people;\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("+----+------+"), std::string::npos);
  EXPECT_NE(output.str().find("| id | name |"), std::string::npos);
  EXPECT_NE(output.str().find("0 row(s)"), std::string::npos);
  EXPECT_TRUE(error.str().empty());
}
