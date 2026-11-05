#include "SqlParserImpl.h"

#include <cctype>
#include <charconv>
#include <cstdint>
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
          isKeyword("OR") || isKeyword("JOIN") || isKeyword("GROUP")) {
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
    while (true) {
      if (!expect(TokenKind::LeftParen)) {
        return syntaxError("Expected '(' before INSERT values");
      }
      std::vector<SqlValue> row;
      while (true) {
        auto value = parseLiteral();
        if (std::holds_alternative<SqlParseError>(value)) {
          return std::get<SqlParseError>(std::move(value));
        }
        row.push_back(std::get<SqlValue>(std::move(value)));
        if (!match(TokenKind::Comma)) {
          break;
        }
      }
      if (!expect(TokenKind::RightParen)) {
        return syntaxError("Expected ')' after INSERT values");
      }
      if (statement.columnNames.size() != row.size()) {
        return syntaxError("INSERT column and value counts must match");
      }
      statement.rows.push_back(std::move(row));
      if (!match(TokenKind::Comma)) {
        break;
      }
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
      auto expression = parseWhereOrExpression();
      if (std::holds_alternative<SqlParseError>(expression)) {
        return std::get<SqlParseError>(std::move(expression));
      }
      statement.where = std::get<WhereExpression>(std::move(expression));
    }

    if (matchKeyword("ORDER")) {
      if (!expectKeyword("BY")) {
        return syntaxError("Expected BY after ORDER");
      }
      while (true) {
        if (current().kind != TokenKind::Identifier) {
          return syntaxError("Expected column name after ORDER BY");
        }
        OrderByColumn order;
        order.columnName = current().text;
        ++position_;
        if (matchKeyword("ASC")) {
          order.ascending = true;
        } else if (matchKeyword("DESC")) {
          order.ascending = false;
        }
        statement.orderBy.push_back(std::move(order));
        if (!match(TokenKind::Comma)) {
          break;
        }
      }
    }
    if (matchKeyword("LIMIT")) {
      auto limit = parseUnsignedInteger();
      if (std::holds_alternative<SqlParseError>(limit)) {
        return std::get<SqlParseError>(std::move(limit));
      }
      statement.limit = std::get<std::uint64_t>(limit);
      if (matchKeyword("OFFSET")) {
        auto offset = parseUnsignedInteger();
        if (std::holds_alternative<SqlParseError>(offset)) {
          return std::get<SqlParseError>(std::move(offset));
        }
        statement.offset = std::get<std::uint64_t>(offset);
      }
    } else if (matchKeyword("OFFSET")) {
      auto offset = parseUnsignedInteger();
      if (std::holds_alternative<SqlParseError>(offset)) {
        return std::get<SqlParseError>(std::move(offset));
      }
      statement.offset = std::get<std::uint64_t>(offset);
    }
    return SqlStatement{std::move(statement)};
  }

  std::variant<WhereExpression, SqlParseError> parseWhereOrExpression() {
    auto first = parseWhereAndExpression();
    if (std::holds_alternative<SqlParseError>(first)) {
      return std::get<SqlParseError>(std::move(first));
    }

    auto expression = std::get<WhereExpression>(std::move(first));
    while (matchKeyword("OR")) {
      auto next = parseWhereAndExpression();
      if (std::holds_alternative<SqlParseError>(next)) {
        return std::get<SqlParseError>(std::move(next));
      }
      if (expression.kind != WhereExpression::Kind::Or) {
        WhereExpression parent;
        parent.kind = WhereExpression::Kind::Or;
        parent.children.push_back(std::move(expression));
        expression = std::move(parent);
      }
      expression.children.push_back(std::get<WhereExpression>(std::move(next)));
    }
    return expression;
  }

  std::variant<WhereExpression, SqlParseError> parseWhereAndExpression() {
    auto first = parseWherePrimary();
    if (std::holds_alternative<SqlParseError>(first)) {
      return std::get<SqlParseError>(std::move(first));
    }

    auto expression = std::get<WhereExpression>(std::move(first));
    while (matchKeyword("AND")) {
      auto next = parseWherePrimary();
      if (std::holds_alternative<SqlParseError>(next)) {
        return std::get<SqlParseError>(std::move(next));
      }
      if (expression.kind != WhereExpression::Kind::And) {
        WhereExpression parent;
        parent.kind = WhereExpression::Kind::And;
        parent.children.push_back(std::move(expression));
        expression = std::move(parent);
      }
      expression.children.push_back(std::get<WhereExpression>(std::move(next)));
    }
    return expression;
  }

  std::variant<WhereExpression, SqlParseError> parseWherePrimary() {
    if (match(TokenKind::LeftParen)) {
      if (++whereNestingDepth_ > 128) {
        --whereNestingDepth_;
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "WHERE expression nesting exceeds 128 levels"};
      }
      if (current().kind == TokenKind::RightParen) {
        --whereNestingDepth_;
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "WHERE expression group cannot be empty"};
      }
      auto expression = parseWhereOrExpression();
      --whereNestingDepth_;
      if (std::holds_alternative<SqlParseError>(expression)) {
        return std::get<SqlParseError>(std::move(expression));
      }
      if (!expect(TokenKind::RightParen)) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected ')' to close WHERE expression"};
      }
      return std::get<WhereExpression>(std::move(expression));
    }

    if (current().kind == TokenKind::RightParen ||
        current().kind == TokenKind::End || isKeyword("AND") ||
        isKeyword("OR")) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected predicate in WHERE expression"};
    }

    if (current().kind != TokenKind::Identifier) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected column name in WHERE predicate"};
    }

    WhereEquals predicate;
    predicate.columnName = current().text;
    ++position_;

    bool isMembership = false;
    bool isNegatedMembership = false;
    if (matchKeyword("NOT")) {
      isMembership = true;
      isNegatedMembership = true;
      if (!matchKeyword("IN")) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected IN after NOT in WHERE predicate"};
      }
    } else if (matchKeyword("IN")) {
      isMembership = true;
    }

    if (isMembership) {
      predicate.comparison = isNegatedMembership
                                 ? ComparisonOperator::NotIn
                                 : ComparisonOperator::In;
      if (!expect(TokenKind::LeftParen)) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected '(' after IN"};
      }
      if (current().kind == TokenKind::RightParen) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "IN list must contain at least one value"};
      }
      while (true) {
        auto value = parseLiteral();
        if (std::holds_alternative<SqlParseError>(value)) {
          return std::get<SqlParseError>(std::move(value));
        }
        predicate.values.push_back(std::get<SqlValue>(std::move(value)));
        if (!match(TokenKind::Comma)) {
          break;
        }
      }
      if (!expect(TokenKind::RightParen)) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected ')' after IN list"};
      }
    } else {
      switch (current().kind) {
      case TokenKind::Equals:
        predicate.comparison = ComparisonOperator::Equal;
        break;
      case TokenKind::NotEquals:
        predicate.comparison = ComparisonOperator::NotEqual;
        break;
      case TokenKind::GreaterThan:
        predicate.comparison = ComparisonOperator::GreaterThan;
        break;
      case TokenKind::LessThan:
        predicate.comparison = ComparisonOperator::LessThan;
        break;
      case TokenKind::GreaterThanOrEqual:
        predicate.comparison = ComparisonOperator::GreaterThanOrEqual;
        break;
      case TokenKind::LessThanOrEqual:
        predicate.comparison = ComparisonOperator::LessThanOrEqual;
        break;
      default:
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected a comparison operator, IN, or NOT IN "
                             "in WHERE predicate"};
      }
      ++position_;
      auto value = parseLiteral();
      if (std::holds_alternative<SqlParseError>(value)) {
        return std::get<SqlParseError>(std::move(value));
      }
      predicate.value = std::get<SqlValue>(std::move(value));
    }

    WhereExpression expression;
    expression.kind = WhereExpression::Kind::Predicate;
    expression.predicate = std::move(predicate);
    return expression;
  }

  std::variant<std::uint64_t, SqlParseError> parseUnsignedInteger() {
    if (current().kind != TokenKind::Integer) {
      return SqlParseError{
          SqlParseErrorCode::SyntaxError,
          "Expected a non-negative integer for LIMIT or OFFSET"};
    }
    std::uint64_t value{};
    const auto &text = current().text;
    const auto conversion =
        std::from_chars(text.data(), text.data() + text.size(), value);
    ++position_;
    if (conversion.ec == std::errc::result_out_of_range) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "LIMIT or OFFSET value is out of range"};
    }
    return value;
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
  std::size_t whereNestingDepth_{0};
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
