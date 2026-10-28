#include "SqlShell.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
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

// Makes terminal control characters visible so values cannot disrupt table layout.
std::string escapeControlCharacters(const std::string &value) {
  std::ostringstream escaped;
  for (const unsigned char ch : value) {
    switch (ch) {
    case '\t':
      escaped << "\\t";
      break;
    case '\n':
      escaped << "\\n";
      break;
    case '\r':
      escaped << "\\r";
      break;
    case '\b':
      escaped << "\\b";
      break;
    case '\f':
      escaped << "\\f";
      break;
    default:
      if (ch < 0x20 || ch == 0x7f) {
        constexpr char hex[] = "0123456789ABCDEF";
        escaped << "\\x" << hex[ch >> 4] << hex[ch & 0x0f];
      } else {
        escaped << static_cast<char>(ch);
      }
      break;
    }
  }
  return escaped.str();
}

std::string formatValue(const unidb::sql::SqlValue &value) {
  std::ostringstream formatted;
  std::visit([&formatted](const auto &item) { formatted << item; }, value);
  return escapeControlCharacters(formatted.str());
}

// Returns an approximate terminal-column width for a Unicode code point.
std::size_t codePointDisplayWidth(std::uint32_t codePoint) {
  if (codePoint == 0 || codePoint < 0x20 ||
      (codePoint >= 0x7f && codePoint < 0xa0)) {
    return 0;
  }

  // Combining marks do not occupy an additional terminal column.
  if ((codePoint >= 0x0300 && codePoint <= 0x036f) ||
      (codePoint >= 0x1ab0 && codePoint <= 0x1aff) ||
      (codePoint >= 0x1dc0 && codePoint <= 0x1dff) ||
      (codePoint >= 0x20d0 && codePoint <= 0x20ff) ||
      (codePoint >= 0xfe20 && codePoint <= 0xfe2f)) {
    return 0;
  }

  // Common East Asian wide/full-width ranges occupy two terminal columns.
  if (codePoint >= 0x1100 &&
      (codePoint <= 0x115f || codePoint == 0x2329 || codePoint == 0x232a ||
       (codePoint >= 0x2e80 && codePoint <= 0xa4cf &&
        codePoint != 0x303f) ||
       (codePoint >= 0xac00 && codePoint <= 0xd7a3) ||
       (codePoint >= 0xf900 && codePoint <= 0xfaff) ||
       (codePoint >= 0xfe10 && codePoint <= 0xfe19) ||
       (codePoint >= 0xfe30 && codePoint <= 0xfe6f) ||
       (codePoint >= 0xff00 && codePoint <= 0xff60) ||
       (codePoint >= 0xffe0 && codePoint <= 0xffe6) ||
       (codePoint >= 0x1f300 && codePoint <= 0x1faff) ||
       (codePoint >= 0x20000 && codePoint <= 0x3fffd))) {
    return 2;
  }
  return 1;
}

std::size_t displayWidth(const std::string &value) {
  std::size_t width = 0;
  for (std::size_t index = 0; index < value.size();) {
    const auto first = static_cast<unsigned char>(value[index]);
    std::uint32_t codePoint = 0;
    std::size_t sequenceLength = 1;

    if (first < 0x80) {
      codePoint = first;
    } else if ((first & 0xe0) == 0xc0 && index + 1 < value.size()) {
      codePoint = first & 0x1f;
      sequenceLength = 2;
    } else if ((first & 0xf0) == 0xe0 && index + 2 < value.size()) {
      codePoint = first & 0x0f;
      sequenceLength = 3;
    } else if ((first & 0xf8) == 0xf0 && index + 3 < value.size()) {
      codePoint = first & 0x07;
      sequenceLength = 4;
    } else {
      // Invalid UTF-8 bytes are rendered as visible escape sequences.
      width += 4;
      ++index;
      continue;
    }

    bool validSequence = true;
    for (std::size_t offset = 1; offset < sequenceLength; ++offset) {
      const auto continuation =
          static_cast<unsigned char>(value[index + offset]);
      if ((continuation & 0xc0) != 0x80) {
        validSequence = false;
        break;
      }
      codePoint = (codePoint << 6) | (continuation & 0x3f);
    }

    if (!validSequence) {
      width += 4;
      ++index;
      continue;
    }

    width += codePointDisplayWidth(codePoint);
    index += sequenceLength;
  }
  return width;
}

void printPadded(const std::string &value, std::size_t width, bool leftAligned,
                 std::ostream &output) {
  const auto valueWidth = displayWidth(value);
  const auto padding = width > valueWidth ? width - valueWidth : 0;
  if (!leftAligned) {
    output << std::string(padding, ' ');
  }
  output << value;
  if (leftAligned) {
    output << std::string(padding, ' ');
  }
}

void printBorder(const std::vector<std::size_t> &widths, std::ostream &output) {
  output << '+';
  for (const auto width : widths) {
    output << std::string(width + 2, '-') << '+';
  }
  output << '\n';
}

// Prints a bordered result table using terminal display widths rather than byte counts.
void printRows(const QueryExecutionService::QueryExecutionResult &result,
               std::ostream &output) {
  std::vector<std::size_t> widths;
  widths.reserve(result.columns.size());
  for (const auto &column : result.columns) {
    widths.push_back(displayWidth(column.name));
  }

  std::vector<std::vector<std::string>> formattedRows;
  formattedRows.reserve(result.rows.size());
  for (const auto &row : result.rows) {
    std::vector<std::string> formattedRow;
    formattedRow.reserve(row.size());
    for (std::size_t index = 0; index < row.size(); ++index) {
      formattedRow.push_back(formatValue(row[index]));
      if (index < widths.size()) {
        widths[index] =
            std::max(widths[index], displayWidth(formattedRow.back()));
      }
    }
    formattedRows.push_back(std::move(formattedRow));
  }

  if (!widths.empty()) {
    printBorder(widths, output);

    output << '|';
    for (std::size_t index = 0; index < result.columns.size(); ++index) {
      output << ' ';
      printPadded(result.columns[index].name, widths[index], true, output);
      output << " |";
    }
    output << '\n';
    printBorder(widths, output);

    for (std::size_t rowIndex = 0; rowIndex < result.rows.size(); ++rowIndex) {
      output << '|';
      for (std::size_t columnIndex = 0; columnIndex < result.columns.size();
           ++columnIndex) {
        output << ' ';
        if (columnIndex < formattedRows[rowIndex].size()) {
          const auto &value = formattedRows[rowIndex][columnIndex];
          const bool isText = std::holds_alternative<std::string>(
              result.rows[rowIndex][columnIndex]);
          printPadded(value, widths[columnIndex], isText, output);
        } else {
          output << std::string(widths[columnIndex], ' ');
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
