#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace SqlAutocomplete {

inline bool isIdentifierCharacter(char ch) {
  const auto value = static_cast<unsigned char>(ch);
  return std::isalnum(value) != 0 || ch == '_';
}

inline bool startsWithIgnoringCase(std::string_view value,
                                   std::string_view prefix) {
  if (prefix.size() > value.size()) {
    return false;
  }
  for (std::size_t index = 0; index < prefix.size(); ++index) {
    if (std::toupper(static_cast<unsigned char>(value[index])) !=
        std::toupper(static_cast<unsigned char>(prefix[index]))) {
      return false;
    }
  }
  return true;
}

inline bool equalsIgnoringCase(std::string_view left,
                               std::string_view right) {
  return left.size() == right.size() &&
         startsWithIgnoringCase(left, right);
}

inline std::vector<std::string>
suggestionsFor(std::string_view prefix) {
  static constexpr std::string_view words[] = {
      "ADD",       "ALL",       "AND",       "AS",        "ASC",
      "AVG",       "BETWEEN",   "BIGINT",    "BOOLEAN",   "BY",
      "CASE",      "CHAR",      "CHARACTER", "CROSS",     "COUNT",
      "CREATE",    "DATE",      "DECIMAL",   "DESC",      "DISTINCT",
      "DOUBLE",    "ELSE",      "END",       "FALSE",     "FROM",
      "GROUP",     "HAVING",    "INNER",     "INSERT",    "INT",
      "INTEGER",   "INTERVAL",  "INTO",      "IS",        "JOIN",
      "LIKE",      "LIMIT",     "MAX",       "MIN",       "NOT",
      "NULL",      "NUMERIC",   "OFFSET",    "ON",        "OR",
      "ORDER",     "PRECISION", "REAL",      "SELECT",    "SMALLINT",
      "SUM",       "TABLE",     "TEXT",      "THEN",      "TIME",
      "TIMESTAMP", "TRUE",      "UNION",     "UPDATE",    "VALUES",
      "VARCHAR",   "VARYING",   "WHEN",      "WHERE"};

  std::vector<std::string> matches;
  if (prefix.empty()) {
    return matches;
  }
  for (const auto word : words) {
    if (startsWithIgnoringCase(word, prefix) &&
        !equalsIgnoringCase(word, prefix)) {
      matches.emplace_back(word);
    }
  }
  return matches;
}

inline std::string prefixAtCursor(const std::string &line,
                                  std::size_t cursor) {
  if (cursor > line.size()) {
    return {};
  }
  std::size_t start = cursor;
  while (start > 0 && isIdentifierCharacter(line[start - 1])) {
    --start;
  }
  return line.substr(start, cursor - start);
}

inline bool replaceToken(std::string &line, std::size_t &cursor,
                         std::string_view completion) {
  if (cursor > line.size() || completion.empty()) {
    return false;
  }
  std::size_t start = cursor;
  while (start > 0 && isIdentifierCharacter(line[start - 1])) {
    --start;
  }
  std::size_t end = cursor;
  while (end < line.size() && isIdentifierCharacter(line[end])) {
    ++end;
  }
  if (start == cursor) {
    return false;
  }
  line.replace(start, end - start, completion);
  cursor = start + completion.size();
  return true;
}

} // namespace SqlAutocomplete
