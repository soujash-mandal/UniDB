#include "SqlShell.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

std::string trim(std::string value) {
  const auto first =
      std::find_if_not(value.begin(), value.end(),
                       [](unsigned char ch) { return std::isspace(ch) != 0; });
  if (first == value.end()) {
    return {};
  }
  const auto last =
      std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
      }).base();
  return std::string(first, last);
}

std::string lower(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

std::string formatValue(const unidb::sql::SqlValue &value) {
  std::ostringstream formatted;
  std::visit([&formatted](const auto &item) { formatted << item; }, value);
  return formatted.str();
}

void printBorder(const std::vector<std::size_t> &widths,
                 std::ostream &output) {
  output << '+';
  for (const auto width : widths) {
    output << std::string(width + 2, '-') << '+';
  }
  output << '\n';
}

void printRows(const QueryExecutionService::QueryExecutionResult &result,
               std::ostream &output) {
  std::vector<std::size_t> widths;
  widths.reserve(result.columns.size());
  for (const auto &column : result.columns) {
    widths.push_back(column.name.size());
  }

  std::vector<std::vector<std::string>> formattedRows;
  formattedRows.reserve(result.rows.size());
  for (const auto &row : result.rows) {
    std::vector<std::string> formattedRow;
    formattedRow.reserve(row.size());
    for (std::size_t index = 0; index < row.size(); ++index) {
      formattedRow.push_back(formatValue(row[index]));
      if (index < widths.size()) {
        widths[index] = std::max(widths[index], formattedRow.back().size());
      }
    }
    formattedRows.push_back(std::move(formattedRow));
  }

  if (!widths.empty()) {
    printBorder(widths, output);

    output << '|';
    for (std::size_t index = 0; index < result.columns.size(); ++index) {
      output << ' ' << std::left << std::setw(static_cast<int>(widths[index]))
             << result.columns[index].name << " |";
    }
    output << '\n';
    printBorder(widths, output);

    for (std::size_t rowIndex = 0; rowIndex < result.rows.size(); ++rowIndex) {
      output << '|';
      for (std::size_t columnIndex = 0;
           columnIndex < result.columns.size(); ++columnIndex) {
        output << ' ';
        if (columnIndex < formattedRows[rowIndex].size()) {
          const auto &value = formattedRows[rowIndex][columnIndex];
          const bool isText = std::holds_alternative<std::string>(
              result.rows[rowIndex][columnIndex]);
          output << (isText ? std::left : std::right)
                 << std::setw(static_cast<int>(widths[columnIndex])) << value;
        } else {
          output << std::setw(static_cast<int>(widths[columnIndex])) << "";
        }
        output << " |";
      }
      output << '\n';
    }

    printBorder(widths, output);
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
