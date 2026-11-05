#include "../SqlShell.h"
#include "../../QueryExecutionService/test/ExecuteQueryAction/ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

TEST(SqlShellTest, PrintsCommandSuccessAndContinuesUntilQuit) {
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
  EXPECT_NE(output.str().find("Rows inserted"), std::string::npos);
  EXPECT_EQ(port.createTableCalls, 1U);
  EXPECT_EQ(port.insertCalls, 1U);
  EXPECT_TRUE(error.str().empty());
}

TEST(SqlShellTest, ReportsInvalidSqlAndContinues) {
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

TEST(SqlShellTest, ClearsScreenWithClsAndContinuesProcessingCommands) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  std::istringstream input("cls\nCREATE TABLE people (id INT);\nquit\n");
  std::ostringstream output;
  std::ostringstream error;
  SqlShell shell(action, input, output, error);

  shell.run();

  EXPECT_NE(output.str().find("\033[2J\033[H"), std::string::npos);
  EXPECT_NE(output.str().find("Table created"), std::string::npos);
  EXPECT_EQ(port.createTableCalls, 1U);
  EXPECT_TRUE(error.str().empty());
}
