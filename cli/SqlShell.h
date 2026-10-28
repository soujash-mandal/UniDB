#pragma once

#include "../QueryExecutionService/actions/ExecuteQueryAction.h"

#include <iosfwd>

class SqlShell {
public:
  SqlShell(QueryExecutionService::ExecuteQueryAction &executeQueryAction,
           std::istream &input, std::ostream &output, std::ostream &error);

  // Reads one SQL statement per line until EOF or a quit command.
  void run();

private:
  QueryExecutionService::ExecuteQueryAction &executeQueryAction;
  std::istream &input;
  std::ostream &output;
  std::ostream &error;
};
