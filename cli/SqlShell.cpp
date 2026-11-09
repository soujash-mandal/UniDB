#include "SqlShell.h"

#include "../QueryExecutionService/domain/types/SqlNumericSemantics.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <cerrno>
#include <termios.h>
#include <unistd.h>

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
    if (first == '\\') {
      escaped += "\\\\";
      ++index;
      continue;
    }
    if (first == '|') {
      escaped += "\\|";
      ++index;
      continue;
    }
    if (first < 0x20 || first == 0x7f) {
      switch (first) {
      case '\t':
        escaped += "\\t";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
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
    if (length == 2 && codePoint >= 0x80 && codePoint <= 0x9f) {
      escaped += "\\u00";
      escaped += hex[codePoint >> 4];
      escaped += hex[codePoint & 0x0f];
      index += length;
      continue;
    }
    escaped.append(value, index, length);
    index += length;
  }
  return escaped;
}

std::string formatValue(const unidb::sql::SqlValue &value) {
  std::ostringstream formatted;
  std::visit(
      [&formatted](const auto &item) {
        using ValueType = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<ValueType, unidb::sql::SqlNumericValue>) {
          formatted << QueryExecutionService::domain::formatSqlNumericValue(
              item);
        } else {
          formatted << item;
        }
      },
      value);
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
      (codePoint >= 0x1f3fb && codePoint <= 0x1f3ff) || codePoint == 0x200d) {
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
      length = 2;
      codePoint = first & 0x1f;
    } else if ((first & 0xf0) == 0xe0) {
      length = 3;
      codePoint = first & 0x0f;
    } else if ((first & 0xf8) == 0xf0) {
      length = 4;
      codePoint = first & 0x07;
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
          (points[index].value >= 0x1f3fb && points[index].value <= 0x1f3ff)) {
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
      printPadded(escapeTerminalText(result.columns[index].name), widths[index],
                  true, output);
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

bool readInteractiveLine(std::string &line,
                         const std::vector<std::string> &history,
                         std::size_t &historyIndex, std::string &historyDraft,
                         std::ostream &output) {
  termios original{};
  if (tcgetattr(STDIN_FILENO, &original) != 0)
    return false;
  termios raw = original;
  raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
    return false;
  struct RestoreTerminal {
    termios settings;
    ~RestoreTerminal() { tcsetattr(STDIN_FILENO, TCSANOW, &settings); }
  } restore{original};

  line.clear();
  std::size_t cursor = 0;
  const auto redraw = [&]() {
    output << "\r\033[2Kunidb> " << line;
    const auto right = displayWidth(line.substr(cursor));
    if (right != 0)
      output << "\033[" << right << 'D';
    output.flush();
  };
  const auto readByte = [](char &value) {
    while (true) {
      const auto count = ::read(STDIN_FILENO, &value, 1);
      if (count == 1)
        return true;
      if (count == 0)
        return false;
      if (errno != EINTR)
        return false;
    }
  };
  const auto previousCharacter = [&line](std::size_t position) {
    if (position == 0)
      return std::size_t{0};
    --position;
    while (position > 0 &&
           (static_cast<unsigned char>(line[position]) & 0xc0) == 0x80)
      --position;
    return position;
  };
  const auto nextCharacter = [&line](std::size_t position) {
    if (position >= line.size())
      return line.size();
    ++position;
    while (position < line.size() &&
           (static_cast<unsigned char>(line[position]) & 0xc0) == 0x80)
      ++position;
    return position;
  };

  redraw();
  while (true) {
    char key = '\0';
    if (!readByte(key)) {
      output << '\n';
      return !line.empty();
    }
    if (key == '\r' || key == '\n') {
      output << "\r\033[2Kunidb> " << line << '\n';
      output.flush();
      return true;
    }
    if (key == 0x1b) {
      char bracket = '\0', direction = '\0';
      if (!readByte(bracket) || bracket != '[' || !readByte(direction))
        continue;
      if (direction == 'A' && !history.empty()) {
        if (historyIndex == history.size())
          historyDraft = line;
        if (historyIndex > 0) {
          line = history[--historyIndex];
          cursor = line.size();
          redraw();
        }
      } else if (direction == 'B' && historyIndex < history.size()) {
        ++historyIndex;
        line = historyIndex == history.size() ? historyDraft
                                              : history[historyIndex];
        cursor = line.size();
        redraw();
      } else if (direction == 'C' && cursor < line.size()) {
        cursor = nextCharacter(cursor);
        redraw();
      } else if (direction == 'D' && cursor > 0) {
        cursor = previousCharacter(cursor);
        redraw();
      }
      continue;
    }
    if (key == 0x04) {
      if (line.empty()) {
        output << '\n';
        return false;
      }
      if (cursor < line.size()) {
        line.erase(cursor, nextCharacter(cursor) - cursor);
        redraw();
      }
      continue;
    }
    if (key == 0x7f || key == '\b') {
      if (cursor > 0) {
        const auto previous = previousCharacter(cursor);
        line.erase(previous, cursor - previous);
        cursor = previous;
        redraw();
      }
      continue;
    }
    if (key == 0x01) {
      cursor = 0;
      redraw();
      continue;
    }
    if (key == 0x05) {
      cursor = line.size();
      redraw();
      continue;
    }
    if (key == 0x15) {
      line.erase(0, cursor);
      cursor = 0;
      redraw();
      continue;
    }
    if (static_cast<unsigned char>(key) >= 0x20) {
      line.insert(cursor, 1, key);
      ++cursor;
      redraw();
    }
  }
}

} // namespace

SqlShell::SqlShell(QueryExecutionService::ExecuteQueryAction &action,
                   std::istream &inputStream, std::ostream &outputStream,
                   std::ostream &errorStream)
    : executeQueryAction(action), input(inputStream), output(outputStream),
      error(errorStream) {}

void SqlShell::run() {
  std::string line;
  std::vector<std::string> history;
  std::size_t historyIndex = 0;
  std::string historyDraft;
  const bool interactiveTerminal = (&input == &std::cin) &&
                                   isatty(STDIN_FILENO) != 0 &&
                                   isatty(STDOUT_FILENO) != 0;
  while (true) {
    bool hasLine = false;
    if (interactiveTerminal) {
      historyIndex = history.size();
      historyDraft.clear();
      hasLine = readInteractiveLine(line, history, historyIndex, historyDraft,
                                    output);
    } else {
      output << "unidb> " << std::flush;
      hasLine = static_cast<bool>(std::getline(input, line));
      if (!hasLine)
        output << '\n';
    }
    if (!hasLine)
      return;

    const std::string statement = trim(line);
    if (interactiveTerminal && !statement.empty() &&
        (history.empty() || history.back() != statement))
      history.push_back(statement);
    const std::string command = lower(statement);
    if (command == "quit" || command == "exit" || command == ".quit") {
      return;
    }
    if (command == "cls") {
      output << "\033[2J\033[H" << std::flush;
      continue;
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
