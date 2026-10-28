#include "../SqlShell.h"
#include "../../QueryExecutionService/adapter/SqlTupleCodec.h"
#include "../../QueryExecutionService/test/ExecuteQueryAction/ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

TEST(SqlShellTest, PrintsCommandSuccessAndContinuesUntilQuit) {
  // Scenario 1: Each supported command is executed through ExecuteQueryAction.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("CREATE TABLE people (id INT, name TEXT);\n"
                           "INSERT INTO people (id, name) VALUES (7, 'Ada');\n"
                           "quit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("Table created"), std::string::npos);
  EXPECT_NE(output.str().find("Row inserted"), std::string::npos);
  EXPECT_EQ(port.createTableCalls, 1U);
  EXPECT_EQ(port.insertCalls, 1U);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, PrintsSelectResultsAsAlignedTable) {
  // Scenario 2: Headers and values are aligned using widths based on their
  // content.
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
  // Scenario 3: Tabs are escaped and multibyte UTF-8 text uses display width.
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
  // Scenario 4: Invalid UTF-8 is visible, and a ZWJ emoji occupies two columns.
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
  EXPECT_NE(output.str().find("👩‍💻"), std::string::npos);
  EXPECT_NE(output.str().find("C1 \\u009B"), std::string::npos);
  EXPECT_NE(output.str().find("a \\| b"), std::string::npos);
  EXPECT_NE(output.str().find("\\\\t"), std::string::npos);
  EXPECT_NE(output.str().find("+----+-----------+"), std::string::npos);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, PrintsHeadersAndBordersForEmptyResultSet) {
  // Scenario 5: An empty SELECT still renders the schema and an empty table.
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

TEST(SqlShellTest, ReportsInvalidSqlAndContinues) {
  // Scenario 6: A statement error does not terminate the interactive loop.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("NOT SQL;\nCREATE TABLE people (id INT);\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_FALSE(error.str().empty());
  EXPECT_NE(output.str().find("Table created"), std::string::npos);
  EXPECT_EQ(port.createTableCalls, 1U);
}

TEST(SqlShellTest, ExitsCleanlyOnEndOfInput) {
  // Scenario 7: EOF exits without requiring a quit command.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input;
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  EXPECT_NO_THROW(shell.run());
  EXPECT_TRUE(error.str().empty());
}
