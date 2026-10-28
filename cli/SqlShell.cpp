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

// Escapes terminal controls and invalid UTF-8 so the measured and printed
// strings always contain the same visible characters.
std::string escapeTerminalText(const std::string &value) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string escaped;
  escaped.reserve(value.size());

  for (std::size_t index = 0; index < value.size();) {
    const auto first = static_cast<unsigned char>(value[index]);
    if (first < 0x20 || first == 0x7f) {
      switch (first) {
      case '\t': escaped += "\\t"; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\b': escaped += "\\b"; break;
      case '\f': escaped += "\\f"; break;
      default:
        escaped += "\\x";
        escaped += hex[first >> 4];
        escaped += hex[first & 0x0f];
        break;
      }
      ++index;
      continue;
    }

    std::size_t length = 0;
    std::uint32_t codePoint = 0;
    if (first < 0x80) {
      length = 1;
      codePoint = first;
    } else if (first >= 0xc2 && first <= 0xdf) {
      length = 2;
      codePoint = first & 0x1f;
    } else if (first >= 0xe0 && first <= 0xef) {
      length = 3;
      codePoint = first & 0x0f;
    } else if (first >= 0xf0 && first <= 0xf4) {
      length = 4;
      codePoint = first & 0x07;
    }

    bool valid = length != 0 && index + length <= value.size();
    for (std::size_t offset = 1; valid && offset < length; ++offset) {
      const auto continuation =
          static_cast<unsigned char>(value[index + offset]);
      valid = (continuation & 0xc0) == 0x80;
      if (valid) {
        codePoint = (codePoint << 6) | (continuation & 0x3f);
      }
    }
    if (valid) {
      valid = !((length == 2 && codePoint < 0x80) ||
                (length == 3 && codePoint < 0x800) ||
                (length == 4 && codePoint < 0x10000) ||
                (codePoint >= 0xd800 && codePoint <= 0xdfff) ||
                codePoint > 0x10ffff);
    }
    if (valid && length == 3) {
      const auto second = static_cast<unsigned char>(value[index + 1]);
      valid = !((first == 0xe0 && second < 0xa0) ||
                (first == 0xed && second >= 0xa0));
    }
    if (valid && length == 4) {
      const auto second = static_cast<unsigned char>(value[index + 1]);
      valid = !((first == 0xf0 && second < 0x90) ||
                (first == 0xf4 && second >= 0x90));
    }

    if (!valid) {
      escaped += "\\x";
      escaped += hex[first >> 4];
      escaped += hex[first & 0x0f];
      ++index;
      continue;
    }
    escaped.append(value, index, length);
    index += length;
  }
  return escaped;
}

std::string formatValue(const unidb::sql::SqlValue &value) {
  std::ostringstream formatted;
  std::visit([&formatted](const auto &item) { formatted << item; }, value);
  return escapeTerminalText(formatted.str());
}

std::size_t codePointDisplayWidth(std::uint32_t codePoint) {
  if (codePoint == 0 || codePoint < 0x20 ||
      (codePoint >= 0x7f && codePoint < 0xa0)) {
    return 0;
  }
  if ((codePoint >= 0x0300 && codePoint <= 0x036f) ||
      (codePoint >= 0x1ab0 && codePoint <= 0x1aff) ||
      (codePoint >= 0x1dc0 && codePoint <= 0x1dff) ||
      (codePoint >= 0x20d0 && codePoint <= 0x20ff) ||
      (codePoint >= 0xfe00 && codePoint <= 0xfe0f) ||
      (codePoint >= 0xfe20 && codePoint <= 0xfe2f) ||
      (codePoint >= 0x1f3fb && codePoint <= 0x1f3ff) ||
      codePoint == 0x200d) {
    return 0;
  }
  if (codePoint >= 0x1100 &&
      (codePoint <= 0x115f || codePoint == 0x2329 || codePoint == 0x232a ||
       (codePoint >= 0x2e80 && codePoint <= 0xa4cf && codePoint != 0x303f) ||
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

struct CodePoint {
  std::uint32_t value;
  std::size_t bytes;
  std::size_t width;
};

std::vector<CodePoint> decodeUtf8(const std::string &value) {
  std::vector<CodePoint> points;
  for (std::size_t index = 0; index < value.size();) {
    const auto first = static_cast<unsigned char>(value[index]);
    std::size_t length = 1;
    std::uint32_t codePoint = first;
    if ((first & 0xe0) == 0xc0) {
      length = 2; codePoint = first & 0x1f;
    } else if ((first & 0xf0) == 0xe0) {
      length = 3; codePoint = first & 0x0f;
    } else if ((first & 0xf8) == 0xf0) {
      length = 4; codePoint = first & 0x07;
    }
    for (std::size_t offset = 1; offset < length; ++offset) {
      codePoint = (codePoint << 6) |
          (static_cast<unsigned char>(value[index + offset]) & 0x3f);
    }
    points.push_back({codePoint, length, codePointDisplayWidth(codePoint)});
    index += length;
  }
  return points;
}

// Approximate terminal columns, treating emoji joined by ZWJ as one cluster.
std::size_t displayWidth(const std::string &value) {
  const auto points = decodeUtf8(value);
  std::size_t width = 0;
  for (std::size_t index = 0; index < points.size();) {
    const auto &point = points[index];
    const bool emoji = point.value >= 0x1f000 && point.value <= 0x1faff;
    if (!emoji) {
      width += point.width;
      ++index;
      continue;
    }

    std::size_t clusterWidth = point.width;
    ++index;
    while (index < points.size()) {
      if (points[index].value == 0xfe0f ||
          (points[index].value >= 0x1f3fb &&
           points[index].value <= 0x1f3ff)) {
        ++index;
        continue;
      }
      if (points[index].value == 0x200d && index + 1 < points.size() &&
          points[index + 1].value >= 0x1f000 &&
          points[index + 1].value <= 0x1faff) {
        clusterWidth = std::max(clusterWidth, points[index + 1].width);
        index += 2;
        continue;
      }
      break;
    }
    width += clusterWidth;
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

// Prints a bordered result table using terminal display widths rather than byte
// counts.
void printRows(const QueryExecutionService::QueryExecutionResult &result,
               std::ostream &output) {
  std::vector<std::size_t> widths;
  widths.reserve(result.columns.size());
  for (const auto &column : result.columns) {
    widths.push_back(displayWidth(escapeTerminalText(column.name)));
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
      printPadded(escapeTerminalText(result.columns[index].name), widths[index], true, output);
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
