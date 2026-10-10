#include "SqlLexer.h"

#include <cctype>
#include <utility>

namespace unidb::sql::implementation {
namespace {

bool isIdentifierStart(char ch) {
  const auto value = static_cast<unsigned char>(ch);
  return std::isalpha(value) != 0 || ch == '_';
}

bool isIdentifierPart(char ch) {
  const auto value = static_cast<unsigned char>(ch);
  return std::isalnum(value) != 0 || ch == '_';
}

} // namespace

LexResult SqlLexer::tokenize(std::string_view sql) const {
  LexResult result;
  std::size_t index = 0;

  const auto add = [&result](TokenKind kind, std::string text,
                             std::size_t position) {
    result.tokens.push_back(Token{kind, std::move(text), position});
  };

  while (index < sql.size()) {
    const char ch = sql[index];
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      ++index;
      continue;
    }

    const std::size_t start = index;
    if (isIdentifierStart(ch)) {
      ++index;
      while (index < sql.size() && isIdentifierPart(sql[index])) {
        ++index;
      }
      add(TokenKind::Identifier, std::string(sql.substr(start, index - start)),
          start);
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
      ++index;
      while (index < sql.size() &&
             std::isdigit(static_cast<unsigned char>(sql[index])) != 0) {
        ++index;
      }
      add(TokenKind::Integer, std::string(sql.substr(start, index - start)),
          start);
      continue;
    }

    if (ch == '\'') {
      ++index;
      std::string value;
      bool closed = false;
      while (index < sql.size()) {
        if (sql[index] == '\'') {
          if (index + 1 < sql.size() && sql[index + 1] == '\'') {
            value.push_back('\'');
            index += 2;
          } else {
            ++index;
            closed = true;
            break;
          }
        } else {
          value.push_back(sql[index]);
          ++index;
        }
      }
      if (!closed) {
        result.error = "Unterminated text literal";
        return result;
      }
      add(TokenKind::String, std::move(value), start);
      continue;
    }

    TokenKind kind = TokenKind::Unsupported;
    switch (ch) {
    case '(':
      kind = TokenKind::LeftParen;
      break;
    case ')':
      kind = TokenKind::RightParen;
      break;
    case ',':
      kind = TokenKind::Comma;
      break;
    case ';':
      kind = TokenKind::Semicolon;
      break;
    case '*':
      kind = TokenKind::Star;
      break;
    case '=':
      kind = TokenKind::Equals;
      break;
    case '-':
      kind = TokenKind::Minus;
      break;
    case '.':
    case '>':
    case '<':
    case '!':
    case '+':
    case '/':
      kind = TokenKind::Unsupported;
      break;
    default:
      result.error = "Unexpected character at position " + std::to_string(index);
      return result;
    }

    add(kind, std::string(1, ch), index);
    if (kind == TokenKind::Unsupported) {
      result.unsupported = true;
    }
    ++index;
  }

  add(TokenKind::End, {}, sql.size());
  return result;
}

} // namespace unidb::sql::implementation
