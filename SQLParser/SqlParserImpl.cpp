#include "SqlParserImpl.h"

#include <cctype>
#include <charconv>
#include <string>
#include <system_error>
#include <utility>

namespace unidb::sql {
namespace {

using implementation::Token;
using implementation::TokenKind;

SqlParseResult syntaxError(std::string message) {
  return SqlParseError{SqlParseErrorCode::SyntaxError, std::move(message)};
}

SqlParseResult unsupported(std::string message) {
  return SqlParseError{SqlParseErrorCode::UnsupportedStatement,
                       std::move(message)};
}

std::string upper(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return value;
}

class Parser {
public:
  explicit Parser(const std::vector<Token> &tokens) : tokens_(tokens) {}

  SqlParseResult parse() {
    if (current().kind == TokenKind::End) {
      return syntaxError("SQL statement is empty");
    }

    SqlParseResult result = syntaxError("Expected a supported SQL statement");
    if (isKeyword("CREATE")) {
      result = parseCreateTable();
    } else if (isKeyword("INSERT")) {
      result = parseInsert();
    } else if (isKeyword("SELECT")) {
      result = parseSelect();
    } else {
      return unsupported("Statement type is outside the V1 SQL subset");
    }

    if (std::holds_alternative<SqlParseError>(result)) {
      return result;
    }
    if (match(TokenKind::Semicolon) && current().kind != TokenKind::End) {
      return syntaxError("Unexpected tokens after semicolon");
    }
    if (current().kind != TokenKind::End) {
      if (current().kind == TokenKind::Unsupported || isKeyword("AND") ||
          isKeyword("OR") || isKeyword("JOIN") || isKeyword("ORDER") ||
          isKeyword("GROUP") || isKeyword("LIMIT")) {
        return unsupported("SQL syntax is outside the V1 supported subset");
      }
      return syntaxError("Unexpected token '" + current().text + "'");
    }
    return result;
  }

private:
  const Token &current() const { return tokens_[position_]; }

  bool isKeyword(std::string_view keyword) const {
    return current().kind == TokenKind::Identifier &&
           upper(current().text) == keyword;
  }

  bool matchKeyword(std::string_view keyword) {
    if (!isKeyword(keyword)) {
      return false;
    }
    ++position_;
    return true;
  }

  bool match(TokenKind kind) {
    if (current().kind != kind) {
      return false;
    }
    ++position_;
    return true;
  }

  bool expectKeyword(std::string_view keyword) { return matchKeyword(keyword); }

  bool expect(TokenKind kind) { return match(kind); }

  SqlParseResult parseCreateTable() {
    ++position_; // CREATE
    if (!expectKeyword("TABLE")) {
      return unsupported("Only CREATE TABLE is supported in V1");
    }
    if (current().kind != TokenKind::Identifier) {
      return syntaxError("Expected table name after CREATE TABLE");
    }
    CreateTableStatement statement;
    statement.tableName = current().text;
    ++position_;
    if (!expect(TokenKind::LeftParen)) {
      return syntaxError("Expected '(' after table name");
    }
    if (current().kind == TokenKind::RightParen) {
      return syntaxError("CREATE TABLE requires at least one column");
    }

    while (true) {
      if (current().kind != TokenKind::Identifier) {
        return syntaxError("Expected column name");
      }
      ColumnDefinition column;
      column.name = current().text;
      ++position_;

      if (isKeyword("INT")) {
        column.type = SqlType::Int;
        ++position_;
      } else if (isKeyword("TEXT")) {
        column.type = SqlType::Text;
        ++position_;
      } else if (current().kind == TokenKind::Identifier) {
        return unsupported("Column type is outside the V1 supported subset");
      } else {
        return syntaxError("Expected INT or TEXT after column name");
      }
      statement.columns.push_back(std::move(column));

      if (!match(TokenKind::Comma)) {
        break;
      }
    }
    if (!expect(TokenKind::RightParen)) {
      return syntaxError("Expected ')' after column definitions");
    }
    return SqlStatement{std::move(statement)};
  }

  SqlParseResult parseInsert() {
    ++position_; // INSERT
    if (!expectKeyword("INTO")) {
      return unsupported("Only INSERT INTO is supported in V1");
    }
    if (current().kind != TokenKind::Identifier) {
      return syntaxError("Expected table name after INSERT INTO");
    }

    InsertStatement statement;
    statement.tableName = current().text;
    ++position_;
    if (!expect(TokenKind::LeftParen)) {
      return syntaxError("Expected '(' before INSERT column list");
    }
    if (current().kind == TokenKind::RightParen) {
      return syntaxError("INSERT requires at least one column");
    }
    while (true) {
      if (current().kind != TokenKind::Identifier) {
        return syntaxError("Expected column name in INSERT column list");
      }
      statement.columnNames.push_back(current().text);
      ++position_;
      if (!match(TokenKind::Comma)) {
        break;
      }
    }
    if (!expect(TokenKind::RightParen)) {
      return syntaxError("Expected ')' after INSERT column list");
    }
    if (!expectKeyword("VALUES")) {
      return syntaxError("Expected VALUES after INSERT column list");
    }
    if (!expect(TokenKind::LeftParen)) {
      return syntaxError("Expected '(' before INSERT values");
    }

    while (true) {
      auto value = parseLiteral();
      if (std::holds_alternative<SqlParseError>(value)) {
        return std::get<SqlParseError>(std::move(value));
      }
      statement.values.push_back(std::get<SqlValue>(std::move(value)));
      if (!match(TokenKind::Comma)) {
        break;
      }
    }
    if (!expect(TokenKind::RightParen)) {
      return syntaxError("Expected ')' after INSERT values");
    }
    if (statement.columnNames.size() != statement.values.size()) {
      return syntaxError("INSERT column and value counts must match");
    }
    return SqlStatement{std::move(statement)};
  }

  SqlParseResult parseSelect() {
    ++position_; // SELECT
    SelectStatement statement;
    if (match(TokenKind::Star)) {
      statement.selectAll = true;
    } else {
      if (current().kind != TokenKind::Identifier) {
        return syntaxError("Expected '*' or column name after SELECT");
      }
      while (true) {
        if (current().kind != TokenKind::Identifier) {
          return syntaxError("Expected column name in SELECT list");
        }
        statement.columnNames.push_back(current().text);
        ++position_;
        if (!match(TokenKind::Comma)) {
          break;
        }
      }
    }
    if (!expectKeyword("FROM")) {
      return syntaxError("Expected FROM after SELECT list");
    }
    if (current().kind != TokenKind::Identifier) {
      return syntaxError("Expected table name after FROM");
    }
    statement.tableName = current().text;
    ++position_;

    if (matchKeyword("WHERE")) {
      if (current().kind != TokenKind::Identifier) {
        return syntaxError("Expected column name after WHERE");
      }
      WhereEquals where;
      where.columnName = current().text;
      ++position_;
      if (current().kind == TokenKind::Unsupported) {
        return unsupported("Only equality predicates are supported in V1");
      }
      if (!expect(TokenKind::Equals)) {
        return syntaxError("Expected '=' in WHERE predicate");
      }
      auto value = parseLiteral();
      if (std::holds_alternative<SqlParseError>(value)) {
        return std::get<SqlParseError>(std::move(value));
      }
      where.value = std::get<SqlValue>(std::move(value));
      statement.where = std::move(where);
    }
    return SqlStatement{std::move(statement)};
  }

  std::variant<SqlValue, SqlParseError> parseLiteral() {
    if (match(TokenKind::Minus)) {
      if (current().kind != TokenKind::Integer) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected digits after '-' in integer literal"};
      }
      const std::string text = "-" + current().text;
      ++position_;
      std::int64_t value{};
      const auto conversion =
          std::from_chars(text.data(), text.data() + text.size(), value);
      if (conversion.ec == std::errc::result_out_of_range) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Integer literal is out of range"};
      }
      return SqlValue{value};
    }
    if (current().kind == TokenKind::Integer) {
      std::int64_t value{};
      const auto &text = current().text;
      const auto conversion =
          std::from_chars(text.data(), text.data() + text.size(), value);
      ++position_;
      if (conversion.ec == std::errc::result_out_of_range) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Integer literal is out of range"};
      }
      return SqlValue{value};
    }
    if (current().kind == TokenKind::String) {
      std::string value = current().text;
      ++position_;
      return SqlValue{std::move(value)};
    }
    if (current().kind == TokenKind::Unsupported || isKeyword("NULL") ||
        isKeyword("TRUE") || isKeyword("FALSE")) {
      return SqlParseError{SqlParseErrorCode::UnsupportedStatement,
                           "Literal type is outside the V1 supported subset"};
    }
    return SqlParseError{SqlParseErrorCode::SyntaxError,
                         "Expected an integer or single-quoted text literal"};
  }

  const std::vector<Token> &tokens_;
  std::size_t position_{0};
};

} // namespace

SqlParseResult SqlParserImpl::parse(std::string_view sql) const {
  auto lexed = lexer_.tokenize(sql);
  if (!lexed.error.empty()) {
    return syntaxError(lexed.error);
  }
  if (lexed.unsupported) {
    return unsupported("Operator or symbol is outside the V1 SQL subset");
  }
  return Parser(lexed.tokens).parse();
}

} // namespace unidb::sql
