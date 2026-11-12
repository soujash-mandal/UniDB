#include "SqlParserImpl.h"

#include "../types/SqlDateTimeSemantics.h"
#include "../types/SqlNumericSemantics.h"
#include "../types/SqlTypeRegistry.h"

#include <cctype>
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace unidb::sql {
namespace {

using implementation::Token;
using implementation::TokenKind;

struct ParsedSqlLiteral {
  SqlValue value;
  std::optional<SqlTypedLiteral> typed;
};

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
      if (const auto *parseError = std::get_if<SqlParseError>(&result)) {
        return *parseError;
      }
      auto &parsedStatement = std::get<SqlStatement>(result);
      if (const auto *firstSelect =
              std::get_if<SelectStatement>(&parsedStatement);
          firstSelect != nullptr && isKeyword("UNION")) {
        const bool unionAllMode = [&]() {
          const auto savedPosition = position_;
          ++position_; // UNION
          const bool hasAll = matchKeyword("ALL");
          position_ = savedPosition;
          return hasAll;
        }();
        UnionAllStatement unionAll;
        UnionStatement unionDistinct;
        if (unionAllMode) {
          unionAll.selects.push_back(*firstSelect);
        } else {
          unionDistinct.selects.push_back(*firstSelect);
        }

        while (matchKeyword("UNION")) {
          const bool nextIsAll = matchKeyword("ALL");
          if (nextIsAll != unionAllMode) {
            return unsupported(
                "Mixing UNION and UNION ALL in one query is not supported");
          }
          if (!isKeyword("SELECT")) {
            return syntaxError(unionAllMode ? "Expected SELECT after UNION ALL"
                                            : "Expected SELECT after UNION");
          }
          auto nextResult = parseSelect();
          if (const auto *error = std::get_if<SqlParseError>(&nextResult)) {
            return *error;
          }
          const auto *nextSelect =
              std::get_if<SelectStatement>(&std::get<SqlStatement>(nextResult));
          if (nextSelect == nullptr) {
            return syntaxError("Set operations require SELECT branches");
          }
          if (unionAllMode) {
            unionAll.selects.push_back(*nextSelect);
          } else {
            unionDistinct.selects.push_back(*nextSelect);
          }
        }
        result = unionAllMode ? SqlStatement{std::move(unionAll)}
                              : SqlStatement{std::move(unionDistinct)};
      }
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
          isKeyword("OR") || isKeyword("JOIN") || isKeyword("INNER")) {
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

  bool isReservedTableAliasKeyword() const {
    return isKeyword("AS") || isKeyword("INNER") || isKeyword("JOIN") ||
           isKeyword("ON") || isKeyword("WHERE") || isKeyword("GROUP") ||
           isKeyword("HAVING") || isKeyword("ORDER") || isKeyword("LIMIT") ||
           isKeyword("OFFSET") || isKeyword("ASC") || isKeyword("DESC") ||
           isKeyword("UNION") || isKeyword("CROSS");
  }

  std::variant<SqlTypeDescriptor, SqlParseError> parseDeclaredType() {
    if (current().kind != TokenKind::Identifier) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected SQL type name"};
    }

    std::string name = upper(current().text);
    ++position_;

    if (name == "DOUBLE") {
      if (!expectKeyword("PRECISION")) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected PRECISION after DOUBLE"};
      }
      name = "DOUBLE PRECISION";
    } else if (name == "CHARACTER" && matchKeyword("VARYING")) {
      name = "CHARACTER VARYING";
    }

    SqlTypeModifier modifier;
    if (match(TokenKind::LeftParen)) {
      const auto parseUnsigned = [this]() -> std::optional<std::uint32_t> {
        if (current().kind != TokenKind::Integer) {
          return std::nullopt;
        }
        std::uint32_t value{};
        const auto &literal = current().text;
        const auto result = std::from_chars(
            literal.data(), literal.data() + literal.size(), value);
        if (result.ec != std::errc{}) {
          return std::nullopt;
        }
        ++position_;
        return value;
      };
      const auto parseSigned = [this]() -> std::optional<std::int32_t> {
        bool negative = match(TokenKind::Minus);
        if (!negative) {
          (void)match(TokenKind::Plus);
        }
        if (current().kind != TokenKind::Integer) {
          return std::nullopt;
        }
        std::int32_t value{};
        const auto &literal = current().text;
        const auto result = std::from_chars(
            literal.data(), literal.data() + literal.size(), value);
        if (result.ec != std::errc{}) {
          return std::nullopt;
        }
        ++position_;
        return negative ? -value : value;
      };

      auto first = parseUnsigned();
      if (!first.has_value()) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected a non-negative type modifier"};
      }
      if (match(TokenKind::Comma)) {
        auto second = parseSigned();
        if (!second.has_value()) {
          return SqlParseError{SqlParseErrorCode::SyntaxError,
                               "Expected NUMERIC scale after ','"};
        }
        modifier.precision = *first;
        modifier.scale = *second;
      } else if (name == "CHAR" || name == "CHARACTER" ||
                 name == "CHARACTER VARYING" || name == "VARCHAR") {
        modifier.length = *first;
      } else if (name == "NUMERIC" || name == "DECIMAL") {
        modifier.precision = *first;
      } else {
        modifier.temporalPrecision = *first;
      }
      if (!expect(TokenKind::RightParen)) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected ')' after SQL type modifier"};
      }
    }

    if (name == "TIME" || name == "TIMESTAMP") {
      if (matchKeyword("WITH")) {
        if (!expectKeyword("TIME") || !expectKeyword("ZONE")) {
          return SqlParseError{SqlParseErrorCode::SyntaxError,
                               "Expected TIME ZONE after WITH"};
        }
        name += " WITH TIME ZONE";
      } else if (matchKeyword("WITHOUT")) {
        if (!expectKeyword("TIME") || !expectKeyword("ZONE")) {
          return SqlParseError{SqlParseErrorCode::SyntaxError,
                               "Expected TIME ZONE after WITHOUT"};
        }
        name += " WITHOUT TIME ZONE";
      }
    }

    try {
      return QueryExecutionService::domain::resolveSqlType(name, modifier);
    } catch (const std::invalid_argument &error) {
      return SqlParseError{SqlParseErrorCode::SyntaxError, error.what()};
    }
  }

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

      auto declaredType = parseDeclaredType();
      if (const auto *error = std::get_if<SqlParseError>(&declaredType)) {
        return *error;
      }
      column.declaredType = std::get<SqlTypeDescriptor>(declaredType);
      // Keep the transitional execution tag populated. The RDB adapter checks
      // declaredType before using it, so unsupported types cannot be stored as
      // an INT/TEXT column by accident.
      column.type = column.declaredType->id == SqlTypeId::Text ? SqlType::Text
                                                               : SqlType::Int;
      if (matchKeyword("NOT")) {
        if (!expectKeyword("NULL")) {
          return syntaxError("Expected NULL after NOT");
        }
        column.nullable = false;
      } else {
        (void)matchKeyword("NULL");
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
      std::vector<std::optional<SqlTypedLiteral>> typedRow;
      while (true) {
        auto value = parseInsertLiteral();
        if (std::holds_alternative<SqlParseError>(value)) {
          return std::get<SqlParseError>(std::move(value));
        }
        auto parsed = std::get<ParsedSqlLiteral>(std::move(value));
        row.push_back(std::move(parsed.value));
        typedRow.push_back(std::move(parsed.typed));
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
      statement.typedLiterals.push_back(std::move(typedRow));
      if (!match(TokenKind::Comma)) {
        break;
      }
    }
    return SqlStatement{std::move(statement)};
  }

  std::variant<ParsedSqlLiteral, SqlParseError> parseInsertLiteral() {
    if (matchKeyword("NULL")) {
      return ParsedSqlLiteral{SqlValue{SqlNullValue{}}, std::nullopt};
    }
    return parseLiteral();
  }

  bool isTextFunctionStart() const {
    if (current().kind != TokenKind::Identifier ||
        position_ + 1 >= tokens_.size() ||
        tokens_[position_ + 1].kind != TokenKind::LeftParen) {
      return false;
    }
    const auto name = upper(current().text);
    return name == "UPPER" || name == "LOWER" || name == "LENGTH";
  }

  bool isAggregateStart() const {
    return current().kind == TokenKind::Identifier &&
           (upper(current().text) == "COUNT" ||
            upper(current().text) == "MIN" || upper(current().text) == "MAX" ||
            upper(current().text) == "SUM" || upper(current().text) == "AVG");
  }

  std::variant<ColumnReference, SqlParseError>
  parseColumnReference(bool requireQualifier) {
    if (current().kind != TokenKind::Identifier) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected column reference"};
    }
    ColumnReference reference;
    const std::string first = current().text;
    ++position_;
    if (match(TokenKind::Dot)) {
      if (current().kind != TokenKind::Identifier) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected column name after '.'"};
      }
      reference.tableName = first;
      reference.columnName = current().text;
      ++position_;
      return reference;
    }
    if (requireQualifier) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "JOIN ON requires table-qualified columns"};
    }
    reference.columnName = first;
    return reference;
  }

  std::variant<TextFunctionExpression, SqlParseError> parseTextFunction() {
    const auto name = upper(current().text);
    ++position_;
    if (!expect(TokenKind::LeftParen)) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected '(' after text function"};
    }
    if (current().kind != TokenKind::Identifier) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           name + " requires a TEXT column argument"};
    }
    auto reference = parseColumnReference(false);
    if (std::holds_alternative<SqlParseError>(reference)) {
      return std::get<SqlParseError>(std::move(reference));
    }
    if (!expect(TokenKind::RightParen)) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected ')' after text function argument"};
    }
    TextFunctionExpression expression;
    expression.function = name == "UPPER"   ? TextFunction::Upper
                          : name == "LOWER" ? TextFunction::Lower
                                            : TextFunction::Length;
    const auto &column = std::get<ColumnReference>(reference);
    expression.columnName = column.tableName.has_value()
                                ? *column.tableName + "." + column.columnName
                                : column.columnName;
    return expression;
  }

  std::variant<AggregateExpression, SqlParseError> parseAggregateCall() {
    const std::string functionName = upper(current().text);
    ++position_;
    if (!expect(TokenKind::LeftParen)) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected '(' after aggregate function"};
    }
    AggregateExpression aggregate;
    if (functionName == "COUNT") {
      aggregate.function = AggregateFunction::Count;
    } else if (functionName == "MIN") {
      aggregate.function = AggregateFunction::Min;
    } else if (functionName == "MAX") {
      aggregate.function = AggregateFunction::Max;
    } else if (functionName == "SUM") {
      aggregate.function = AggregateFunction::Sum;
    } else {
      aggregate.function = AggregateFunction::Avg;
    }
    if (match(TokenKind::Star)) {
      if (aggregate.function != AggregateFunction::Count) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Only COUNT supports '*'"};
      }
    } else {
      if (current().kind != TokenKind::Identifier) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected column name in aggregate function"};
      }
      aggregate.columnName = current().text;
      ++position_;
    }
    if (!expect(TokenKind::RightParen)) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected ')' after aggregate argument"};
    }
    if (matchKeyword("AS")) {
      if (current().kind != TokenKind::Identifier) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected alias after AS"};
      }
      aggregate.alias = current().text;
      ++position_;
    }
    return aggregate;
  }

  std::variant<CaseExpression, SqlParseError> parseCaseExpression() {
    ++position_; // CASE
    CaseExpression expression;
    while (matchKeyword("WHEN")) {
      auto condition = parseWhereOrExpression();
      if (std::holds_alternative<SqlParseError>(condition)) {
        return std::get<SqlParseError>(std::move(condition));
      }
      if (!expectKeyword("THEN")) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected THEN after CASE WHEN condition"};
      }
      auto value = parseLiteral();
      if (std::holds_alternative<SqlParseError>(value)) {
        return std::get<SqlParseError>(std::move(value));
      }
      auto parsedValue = std::get<ParsedSqlLiteral>(std::move(value));
      expression.branches.push_back(
          {std::get<WhereExpression>(std::move(condition)),
           std::move(parsedValue.value), std::move(parsedValue.typed)});
    }
    if (expression.branches.empty()) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "CASE requires at least one WHEN branch"};
    }
    if (!expectKeyword("ELSE")) {
      return SqlParseError{
          SqlParseErrorCode::SyntaxError,
          "CASE requires ELSE because SQL NULL is unsupported"};
    }
    auto elseValue = parseLiteral();
    if (std::holds_alternative<SqlParseError>(elseValue)) {
      return std::get<SqlParseError>(std::move(elseValue));
    }
    auto parsedElse = std::get<ParsedSqlLiteral>(std::move(elseValue));
    expression.elseResult = std::move(parsedElse.value);
    expression.typedElseResult = std::move(parsedElse.typed);
    if (!expectKeyword("END")) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Expected END after CASE expression"};
    }
    const bool resultIsInt =
        std::holds_alternative<std::int64_t>(expression.elseResult);
    for (const auto &branch : expression.branches) {
      if (std::holds_alternative<std::int64_t>(branch.result) != resultIsInt) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "CASE result values must have the same type"};
      }
    }
    return expression;
  }

  std::variant<IntegerExpression, SqlParseError> parseIntegerPrimary() {
    if (match(TokenKind::LeftParen)) {
      if (++expressionNestingDepth_ > 128) {
        --expressionNestingDepth_;
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Integer expression nesting exceeds 128 levels"};
      }
      auto expression = parseIntegerAdditive();
      --expressionNestingDepth_;
      if (std::holds_alternative<SqlParseError>(expression)) {
        return std::get<SqlParseError>(std::move(expression));
      }
      if (!expect(TokenKind::RightParen)) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected ')' to close integer expression"};
      }
      return std::get<IntegerExpression>(std::move(expression));
    }
    if (current().kind == TokenKind::Floating) {
      IntegerExpression expression;
      try {
        expression.numericLiteral =
            QueryExecutionService::domain::parseSqlNumericLiteral(
                current().text);
      } catch (const std::exception &error) {
        return SqlParseError{SqlParseErrorCode::SyntaxError, error.what()};
      }
      ++position_;
      return expression;
    }
    if (current().kind == TokenKind::Integer) {
      const auto &text = current().text;
      std::int64_t value{};
      const auto conversion =
          std::from_chars(text.data(), text.data() + text.size(), value);
      ++position_;
      if (conversion.ec == std::errc::result_out_of_range) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Integer literal is out of range"};
      }
      IntegerExpression expression;
      expression.literal = value;
      return expression;
    }
    if (current().kind == TokenKind::Identifier) {
      auto reference = parseColumnReference(false);
      if (std::holds_alternative<SqlParseError>(reference)) {
        return std::get<SqlParseError>(std::move(reference));
      }
      const auto &column = std::get<ColumnReference>(reference);
      IntegerExpression expression;
      expression.kind = IntegerExpression::Kind::Column;
      expression.columnName = column.tableName.has_value()
                                  ? *column.tableName + "." + column.columnName
                                  : column.columnName;
      return expression;
    }
    return SqlParseError{SqlParseErrorCode::SyntaxError,
                         "Expected an integer literal, column, or '('"};
  }

  std::variant<IntegerExpression, SqlParseError> parseIntegerUnary() {
    if (current().kind != TokenKind::Plus &&
        current().kind != TokenKind::Minus) {
      return parseIntegerPrimary();
    }

    const auto operation = current().kind == TokenKind::Plus
                               ? IntegerUnaryOperator::Positive
                               : IntegerUnaryOperator::Negative;
    ++position_;

    if (current().kind == TokenKind::Floating) {
      std::string text = current().text;
      if (operation == IntegerUnaryOperator::Negative) {
        text.insert(text.begin(), '-');
      }
      ++position_;
      IntegerExpression expression;
      try {
        expression.numericLiteral =
            QueryExecutionService::domain::parseSqlNumericLiteral(text);
      } catch (const std::exception &error) {
        return SqlParseError{SqlParseErrorCode::SyntaxError, error.what()};
      }
      return expression;
    }

    // A negative literal may use the full signed range, including INT64_MIN.
    if (operation == IntegerUnaryOperator::Negative &&
        current().kind == TokenKind::Integer) {
      const std::string text = "-" + current().text;
      ++position_;
      std::int64_t value{};
      const auto conversion =
          std::from_chars(text.data(), text.data() + text.size(), value);
      if (conversion.ec == std::errc::result_out_of_range) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Integer literal is out of range"};
      }
      IntegerExpression expression;
      expression.literal = value;
      return expression;
    }

    if (++unaryNestingDepth_ > 128) {
      --unaryNestingDepth_;
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Integer unary operator nesting exceeds 128 levels"};
    }
    auto operand = parseIntegerUnary();
    --unaryNestingDepth_;
    if (std::holds_alternative<SqlParseError>(operand)) {
      return std::get<SqlParseError>(std::move(operand));
    }

    IntegerExpression expression;
    expression.kind = IntegerExpression::Kind::Unary;
    expression.unaryOperation = operation;
    expression.operand = std::make_shared<IntegerExpression>(
        std::get<IntegerExpression>(std::move(operand)));
    return expression;
  }

  std::variant<IntegerExpression, SqlParseError> parseIntegerMultiplicative() {
    auto first = parseIntegerUnary();
    if (std::holds_alternative<SqlParseError>(first)) {
      return std::get<SqlParseError>(std::move(first));
    }
    auto expression = std::get<IntegerExpression>(std::move(first));
    while (current().kind == TokenKind::Star ||
           current().kind == TokenKind::Slash) {
      const auto operation = current().kind == TokenKind::Star
                                 ? IntegerBinaryOperator::Multiply
                                 : IntegerBinaryOperator::Divide;
      ++position_;
      auto next = parseIntegerUnary();
      if (std::holds_alternative<SqlParseError>(next)) {
        return std::get<SqlParseError>(std::move(next));
      }
      IntegerExpression parent;
      parent.kind = IntegerExpression::Kind::Binary;
      parent.operation = operation;
      parent.left = std::make_shared<IntegerExpression>(std::move(expression));
      parent.right = std::make_shared<IntegerExpression>(
          std::get<IntegerExpression>(std::move(next)));
      expression = std::move(parent);
    }
    return expression;
  }

  std::variant<IntegerExpression, SqlParseError> parseIntegerAdditive() {
    auto first = parseIntegerMultiplicative();
    if (std::holds_alternative<SqlParseError>(first)) {
      return std::get<SqlParseError>(std::move(first));
    }
    auto expression = std::get<IntegerExpression>(std::move(first));
    while (current().kind == TokenKind::Plus ||
           current().kind == TokenKind::Minus) {
      const auto operation = current().kind == TokenKind::Plus
                                 ? IntegerBinaryOperator::Add
                                 : IntegerBinaryOperator::Subtract;
      ++position_;
      auto next = parseIntegerMultiplicative();
      if (std::holds_alternative<SqlParseError>(next)) {
        return std::get<SqlParseError>(std::move(next));
      }
      IntegerExpression parent;
      parent.kind = IntegerExpression::Kind::Binary;
      parent.operation = operation;
      parent.left = std::make_shared<IntegerExpression>(std::move(expression));
      parent.right = std::make_shared<IntegerExpression>(
          std::get<IntegerExpression>(std::move(next)));
      expression = std::move(parent);
    }
    return expression;
  }

  std::variant<IntegerExpression, SqlParseError> parseIntegerExpression() {
    return parseIntegerAdditive();
  }

  SqlParseResult parseSelect() {
    ++position_; // SELECT
    SelectStatement statement;
    statement.distinct = matchKeyword("DISTINCT");
    if (isAggregateStart()) {
      auto aggregate = parseAggregateCall();
      if (std::holds_alternative<SqlParseError>(aggregate)) {
        return std::get<SqlParseError>(std::move(aggregate));
      }
      statement.aggregate = std::get<AggregateExpression>(std::move(aggregate));
    } else if (match(TokenKind::Star)) {
      statement.selectAll = true;
      if (matchKeyword("AS")) {
        return syntaxError("SELECT * cannot have a column alias");
      }
    } else {
      if (current().kind != TokenKind::Identifier &&
          current().kind != TokenKind::Integer &&
          current().kind != TokenKind::Floating &&
          current().kind != TokenKind::LeftParen &&
          current().kind != TokenKind::Minus &&
          current().kind != TokenKind::Plus) {
        return syntaxError("Expected '*', aggregate, CASE, column, or numeric "
                           "expression after SELECT");
      }
      while (true) {
        if (isAggregateStart()) {
          auto aggregate = parseAggregateCall();
          if (std::holds_alternative<SqlParseError>(aggregate)) {
            return std::get<SqlParseError>(std::move(aggregate));
          }
          statement.aggregate =
              std::get<AggregateExpression>(std::move(aggregate));
          break;
        }

        if (isKeyword("CASE")) {
          auto caseExpression = parseCaseExpression();
          if (std::holds_alternative<SqlParseError>(caseExpression)) {
            return std::get<SqlParseError>(std::move(caseExpression));
          }
          statement.columnNames.emplace_back();
          statement.caseExpressions.push_back(
              std::get<CaseExpression>(std::move(caseExpression)));
          statement.integerExpressions.push_back(std::nullopt);
          statement.textFunctions.push_back(std::nullopt);
        } else if (isTextFunctionStart()) {
          auto textFunction = parseTextFunction();
          if (std::holds_alternative<SqlParseError>(textFunction)) {
            return std::get<SqlParseError>(std::move(textFunction));
          }
          statement.columnNames.emplace_back();
          statement.caseExpressions.push_back(std::nullopt);
          statement.integerExpressions.push_back(std::nullopt);
          statement.textFunctions.push_back(
              std::get<TextFunctionExpression>(std::move(textFunction)));
        } else {
          auto expression = parseIntegerExpression();
          if (std::holds_alternative<SqlParseError>(expression)) {
            return std::get<SqlParseError>(std::move(expression));
          }
          auto parsedExpression =
              std::get<IntegerExpression>(std::move(expression));
          if (parsedExpression.kind == IntegerExpression::Kind::Column) {
            statement.columnNames.push_back(parsedExpression.columnName);
            statement.integerExpressions.push_back(std::nullopt);
          } else {
            statement.columnNames.emplace_back();
            statement.integerExpressions.push_back(std::move(parsedExpression));
          }
          statement.caseExpressions.push_back(std::nullopt);
          statement.textFunctions.push_back(std::nullopt);
        }
        std::optional<std::string> alias;
        if (matchKeyword("AS")) {
          if (current().kind != TokenKind::Identifier) {
            return syntaxError("Expected alias after AS");
          }
          alias = current().text;
          ++position_;
        }
        statement.columnAliases.push_back(std::move(alias));
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
    if (matchKeyword("AS")) {
      if (current().kind != TokenKind::Identifier ||
          isReservedTableAliasKeyword()) {
        return syntaxError("Expected table alias after AS");
      }
      statement.tableAlias = current().text;
      ++position_;
    } else if (current().kind == TokenKind::Identifier &&
               !isReservedTableAliasKeyword()) {
      statement.tableAlias = current().text;
      ++position_;
    }

    while (matchKeyword("INNER")) {
      if (!expectKeyword("JOIN"))
        return syntaxError("Expected JOIN after INNER");
      if (current().kind != TokenKind::Identifier)
        return syntaxError("Expected joined table name after INNER JOIN");
      InnerJoinClause join;
      join.tableName = current().text;
      ++position_;
      if (matchKeyword("AS")) {
        if (current().kind != TokenKind::Identifier ||
            isReservedTableAliasKeyword())
          return syntaxError("Expected table alias after AS");
        join.tableAlias = current().text;
        ++position_;
      } else if (current().kind == TokenKind::Identifier &&
                 !isReservedTableAliasKeyword()) {
        join.tableAlias = current().text;
        ++position_;
      }
      if (!expectKeyword("ON"))
        return syntaxError("Expected ON after joined table name");
      auto left = parseColumnReference(true);
      if (std::holds_alternative<SqlParseError>(left))
        return std::get<SqlParseError>(std::move(left));
      join.left = std::get<ColumnReference>(std::move(left));
      switch (current().kind) {
      case TokenKind::Equals:
        join.comparison = ComparisonOperator::Equal;
        break;
      case TokenKind::NotEquals:
        join.comparison = ComparisonOperator::NotEqual;
        break;
      case TokenKind::GreaterThan:
        join.comparison = ComparisonOperator::GreaterThan;
        break;
      case TokenKind::LessThan:
        join.comparison = ComparisonOperator::LessThan;
        break;
      case TokenKind::GreaterThanOrEqual:
        join.comparison = ComparisonOperator::GreaterThanOrEqual;
        break;
      case TokenKind::LessThanOrEqual:
        join.comparison = ComparisonOperator::LessThanOrEqual;
        break;
      default:
        return syntaxError("Expected comparison operator in INNER JOIN ON");
      }
      ++position_;
      auto right = parseColumnReference(true);
      if (std::holds_alternative<SqlParseError>(right))
        return std::get<SqlParseError>(std::move(right));
      join.right = std::get<ColumnReference>(std::move(right));
      statement.innerJoins.push_back(std::move(join));
    }
    if (matchKeyword("CROSS")) {
      if (!statement.innerJoins.empty()) {
        return unsupported("Mixing CROSS JOIN and INNER JOIN is not supported");
      }
      if (!expectKeyword("JOIN")) {
        return syntaxError("Expected JOIN after CROSS");
      }
      if (current().kind != TokenKind::Identifier) {
        return syntaxError("Expected joined table name after CROSS JOIN");
      }
      CrossJoinClause join;
      join.tableName = current().text;
      ++position_;
      if (matchKeyword("AS")) {
        if (current().kind != TokenKind::Identifier ||
            isReservedTableAliasKeyword()) {
          return syntaxError("Expected table alias after AS");
        }
        join.tableAlias = current().text;
        ++position_;
      } else if (current().kind == TokenKind::Identifier &&
                 !isReservedTableAliasKeyword()) {
        join.tableAlias = current().text;
        ++position_;
      }
      statement.crossJoin = std::move(join);
      if (isKeyword("CROSS") || isKeyword("INNER")) {
        return unsupported("Only one CROSS JOIN is supported per SELECT");
      }
    }
    if (isKeyword("JOIN")) {
      return unsupported(
          "Only explicit INNER JOIN or CROSS JOIN syntax is supported");
    }

    if (matchKeyword("WHERE")) {
      auto expression = parseWhereOrExpression();
      if (std::holds_alternative<SqlParseError>(expression)) {
        return std::get<SqlParseError>(std::move(expression));
      }
      statement.where = std::get<WhereExpression>(std::move(expression));
    }

    if (matchKeyword("GROUP")) {
      if (!expectKeyword("BY")) {
        return syntaxError("Expected BY after GROUP");
      }
      while (true) {
        if (current().kind != TokenKind::Identifier) {
          return syntaxError("Expected column name after GROUP BY");
        }
        statement.groupBy.push_back(current().text);
        ++position_;
        if (!match(TokenKind::Comma)) {
          break;
        }
      }
    }

    if (matchKeyword("HAVING")) {
      if (!isAggregateStart()) {
        return syntaxError("HAVING must start with an aggregate function");
      }
      auto aggregate = parseAggregateCall();
      if (std::holds_alternative<SqlParseError>(aggregate)) {
        return std::get<SqlParseError>(std::move(aggregate));
      }
      HavingExpression having;
      having.aggregate = std::get<AggregateExpression>(std::move(aggregate));
      switch (current().kind) {
      case TokenKind::Equals:
        having.comparison = ComparisonOperator::Equal;
        break;
      case TokenKind::NotEquals:
        having.comparison = ComparisonOperator::NotEqual;
        break;
      case TokenKind::GreaterThan:
        having.comparison = ComparisonOperator::GreaterThan;
        break;
      case TokenKind::LessThan:
        having.comparison = ComparisonOperator::LessThan;
        break;
      case TokenKind::GreaterThanOrEqual:
        having.comparison = ComparisonOperator::GreaterThanOrEqual;
        break;
      case TokenKind::LessThanOrEqual:
        having.comparison = ComparisonOperator::LessThanOrEqual;
        break;
      default:
        return syntaxError("Expected comparison operator in HAVING");
      }
      ++position_;
      auto value = parseLiteral();
      if (std::holds_alternative<SqlParseError>(value)) {
        return std::get<SqlParseError>(std::move(value));
      }
      auto parsed = std::get<ParsedSqlLiteral>(std::move(value));
      having.value = std::move(parsed.value);
      having.typedLiteral = std::move(parsed.typed);
      statement.having = std::move(having);
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
        if (match(TokenKind::Dot)) {
          if (current().kind != TokenKind::Identifier) {
            return syntaxError("Expected column name after ORDER BY qualifier");
          }
          order.columnName += "." + current().text;
          ++position_;
        }
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
    if (match(TokenKind::Dot)) {
      if (current().kind != TokenKind::Identifier) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected column name after '.' in WHERE"};
      }
      predicate.columnName += "." + current().text;
      ++position_;
    }

    bool isMembership = false;
    bool isNegatedMembership = false;
    bool isRange = false;
    bool isNegatedRange = false;
    bool isLike = false;
    bool isNegatedLike = false;
    if (matchKeyword("NOT")) {
      if (matchKeyword("IN")) {
        isMembership = true;
        isNegatedMembership = true;
      } else if (matchKeyword("BETWEEN")) {
        isRange = true;
        isNegatedRange = true;
      } else if (matchKeyword("LIKE")) {
        isLike = true;
        isNegatedLike = true;
      } else {
        return SqlParseError{
            SqlParseErrorCode::SyntaxError,
            "Expected IN, BETWEEN, or LIKE after NOT in WHERE predicate"};
      }
    } else if (matchKeyword("IN")) {
      isMembership = true;
    } else if (matchKeyword("BETWEEN")) {
      isRange = true;
    } else if (matchKeyword("LIKE")) {
      isLike = true;
    }

    if (isLike) {
      predicate.comparison = isNegatedLike ? ComparisonOperator::NotLike
                                           : ComparisonOperator::Like;
      auto pattern = parseLiteral();
      if (std::holds_alternative<SqlParseError>(pattern)) {
        return std::get<SqlParseError>(std::move(pattern));
      }
      auto parsed = std::get<ParsedSqlLiteral>(std::move(pattern));
      predicate.value = std::move(parsed.value);
      predicate.typedLiteral = std::move(parsed.typed);
    } else if (isRange) {
      predicate.comparison = isNegatedRange ? ComparisonOperator::NotBetween
                                            : ComparisonOperator::Between;
      auto lowerBound = parseLiteral();
      if (std::holds_alternative<SqlParseError>(lowerBound)) {
        return std::get<SqlParseError>(std::move(lowerBound));
      }
      auto parsedLower = std::get<ParsedSqlLiteral>(std::move(lowerBound));
      predicate.value = std::move(parsedLower.value);
      predicate.typedLiteral = std::move(parsedLower.typed);
      if (!matchKeyword("AND")) {
        return SqlParseError{SqlParseErrorCode::SyntaxError,
                             "Expected AND between BETWEEN range bounds"};
      }
      auto upperBound = parseLiteral();
      if (std::holds_alternative<SqlParseError>(upperBound)) {
        return std::get<SqlParseError>(std::move(upperBound));
      }
      auto parsedUpper = std::get<ParsedSqlLiteral>(std::move(upperBound));
      predicate.upperBound = std::move(parsedUpper.value);
      predicate.typedUpperBound = std::move(parsedUpper.typed);
    } else if (isMembership) {
      predicate.comparison = isNegatedMembership ? ComparisonOperator::NotIn
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
        auto parsed = std::get<ParsedSqlLiteral>(std::move(value));
        predicate.values.push_back(std::move(parsed.value));
        predicate.typedValues.push_back(std::move(parsed.typed));
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
        return SqlParseError{
            SqlParseErrorCode::SyntaxError,
            "Expected a comparison operator, IN, BETWEEN, or LIKE "
            "in WHERE predicate"};
      }
      ++position_;
      auto value = parseLiteral();
      if (std::holds_alternative<SqlParseError>(value)) {
        return std::get<SqlParseError>(std::move(value));
      }
      auto parsed = std::get<ParsedSqlLiteral>(std::move(value));
      predicate.value = std::move(parsed.value);
      predicate.typedLiteral = std::move(parsed.typed);
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

  std::variant<ParsedSqlLiteral, SqlParseError> parseLiteral() {
    if (current().kind == TokenKind::Identifier) {
      const auto typeName = upper(current().text);
      if (typeName == "DATE" || typeName == "TIME" || typeName == "TIMESTAMP" ||
          typeName == "TIMETZ" || typeName == "TIMESTAMPTZ" ||
          typeName == "INTERVAL") {
        auto declaredType = parseDeclaredType();
        if (const auto *error = std::get_if<SqlParseError>(&declaredType)) {
          return *error;
        }
        const auto descriptor = std::get<SqlTypeDescriptor>(declaredType);
        const auto typeId = descriptor.id;
        if (typeId != SqlTypeId::Date && typeId != SqlTypeId::Time &&
            typeId != SqlTypeId::TimeWithTimeZone &&
            typeId != SqlTypeId::Timestamp &&
            typeId != SqlTypeId::TimestampWithTimeZone &&
            typeId != SqlTypeId::Interval) {
          return SqlParseError{SqlParseErrorCode::SyntaxError,
                               "Expected a supported date/time literal type"};
        }
        if (current().kind != TokenKind::String) {
          return SqlParseError{SqlParseErrorCode::SyntaxError,
                               "Expected a quoted string after date/time type"};
        }
        try {
          SqlTypedLiteral typed{
              descriptor, current().text,
              QueryExecutionService::domain::parseSqlDateTimeLiteral(
                  descriptor, current().text)};
          ++position_;
          return ParsedSqlLiteral{SqlValue{typed.literal}, std::move(typed)};
        } catch (const std::exception &error) {
          return SqlParseError{SqlParseErrorCode::SyntaxError, error.what()};
        }
      }
    }
    const bool negative = match(TokenKind::Minus);
    const bool explicitPlus = !negative && match(TokenKind::Plus);
    if (current().kind == TokenKind::Floating) {
      std::string text = current().text;
      if (negative) {
        text.insert(text.begin(), '-');
      }
      ++position_;
      try {
        return ParsedSqlLiteral{
            SqlValue{
                QueryExecutionService::domain::parseSqlNumericLiteral(text)},
            std::nullopt};
      } catch (const std::exception &error) {
        return SqlParseError{SqlParseErrorCode::SyntaxError, error.what()};
      }
    }
    if (negative) {
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
      return ParsedSqlLiteral{SqlValue{value}, std::nullopt};
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
      return ParsedSqlLiteral{SqlValue{value}, std::nullopt};
    }
    if (explicitPlus && current().kind != TokenKind::Integer) {
      return SqlParseError{SqlParseErrorCode::SyntaxError,
                           "Unary '+' requires a numeric literal"};
    }
    if (current().kind == TokenKind::String) {
      std::string value = current().text;
      ++position_;
      return ParsedSqlLiteral{SqlValue{std::move(value)}, std::nullopt};
    }
    if (matchKeyword("TRUE")) {
      return ParsedSqlLiteral{SqlValue{true}, std::nullopt};
    }
    if (matchKeyword("FALSE")) {
      return ParsedSqlLiteral{SqlValue{false}, std::nullopt};
    }
    if (current().kind == TokenKind::Unsupported || isKeyword("NULL")) {
      return SqlParseError{SqlParseErrorCode::UnsupportedStatement,
                           "Literal type is outside the V1 supported subset"};
    }
    return SqlParseError{SqlParseErrorCode::SyntaxError,
                         "Expected an integer or single-quoted text literal"};
  }

  const std::vector<Token> &tokens_;
  std::size_t position_{0};
  std::size_t whereNestingDepth_{0};
  std::size_t expressionNestingDepth_{0};
  std::size_t unaryNestingDepth_{0};
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
