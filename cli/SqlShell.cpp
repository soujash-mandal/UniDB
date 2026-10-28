#include "SqlShell.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

namespace {

std::string trim(std::string value) {
  const auto first = std::find_if_not(value.begin(), value.end(),
                                      [](unsigned char ch) {
                                        return std::isspace(ch) != 0;
                                      });
  if (first == value.end()) {
    return {};
  }
  const auto last = std::find_if_not(value.rbegin(), value.rend(),
                                     [](unsigned char ch) {
                                       return std::isspace(ch) != 0;
                                     }).base();
  return std::string(first, last);
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) {
                   return static_cast<char>(std::tolower(ch));
                 });
  return value;
}

void printRows(const QueryExecutionService::QueryExecutionResult &result,
               std::ostream &output) {
  for (std::size_t index = 0; index < result.columns.size(); ++index) {
    if (index != 0) {
      output << " | ";
    }
    output << result.columns[index].name;
  }
  output << '\n';

  for (const auto &row : result.rows) {
    for (std::size_t index = 0; index < row.size(); ++index) {
      if (index != 0) {
        output << " | ";
      }
      std::visit([&output](const auto &value) { output << value; }, row[index]);
    }
    output << '\n';
  }
  output << result.rows.size() << " row(s)\n";
}

} // namespace

SqlShell::SqlShell(QueryExecutionService::ExecuteQueryAction &action,
                   std::istream &inputStream, std::ostream &outputStream,
                   std::ostream &errorStream)
    : executeQueryAction(action), input(inputStream), output(outputStream),
      error(errorStream) {}

void SqlShell::run() {
  std::string line;
  while (true) {
    output << "unidb> " << std::flush;
    if (!std::getline(input, line)) {
      output << '\n';
      return;
    }

    const std::string statement = trim(line);
    const std::string command = lower(statement);
    if (command == "quit" || command == "exit" || command == ".quit") {
      return;
    }
    if (statement.empty()) {
      continue;
    }

    const auto outcome = executeQueryAction.execute(statement);
    if (const auto *failure =
            std::get_if<QueryExecutionService::QueryExecutionError>(&outcome)) {
      error << "Error: " << failure->message << '\n';
      continue;
    }

    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(outcome);
    if (result.kind == QueryExecutionService::QueryExecutionResultKind::Rows) {
      printRows(result, output);
    } else {
      output << result.message << '\n';
    }
  }
}
