#include "../../QueryExecutionService/test/ExecuteQueryAction/ExecuteQueryActionTestSupport.h"
#include "../SqlShell.h"
#include "../../QueryExecutionService/adapter/SqlTupleCodec.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

TEST(SqlShellTest, PrintsCommandSuccessAndContinuesUntilQuit) {
  // Scenario 1: Each supported command is executed through ExecuteQueryAction.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input(
      "CREATE TABLE people (id INT, name TEXT);\n"
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

TEST(SqlShellTest, PrintsSelectColumnNamesAndValues) {
  // Scenario 2: SELECT * output contains schema names and decoded values.
  FakeQesRdbServicePort port;
  port.rows.push_back(QueryExecutionService::SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("Ada")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("SELECT * FROM people;\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("id"), std::string::npos);
  EXPECT_NE(output.str().find("name"), std::string::npos);
  EXPECT_NE(output.str().find("7"), std::string::npos);
  EXPECT_NE(output.str().find("Ada"), std::string::npos);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, ReportsInvalidSqlAndContinues) {
  // Scenario 3: A statement error does not terminate the interactive loop.
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
  // Scenario 4: EOF exits without requiring a quit command.
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
