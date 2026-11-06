#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace unidb::sql::implementation {

enum class TokenKind {
  Identifier,
  Integer,
  String,
  LeftParen,
  RightParen,
  Comma,
  Dot,
  Semicolon,
  Star,
  Equals,
  NotEquals,
  GreaterThan,
  LessThan,
  GreaterThanOrEqual,
  LessThanOrEqual,
  Minus,
  Plus,
  Slash,
  Unsupported,
  End
};

struct Token {
  TokenKind kind;
  std::string text;
  std::size_t position;
};

struct LexResult {
  std::vector<Token> tokens;
  bool unsupported{false};
  std::string error;
};

class SqlLexer {
public:
  LexResult tokenize(std::string_view sql) const;
};

} // namespace unidb::sql::implementation
