#include "ScalarExpressionEvaluator.h"

#include "../types/SqlNumericSemantics.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace QueryExecutionService::domain {
using unidb::sql::IntegerBinaryOperator;
using unidb::sql::IntegerExpression;
using unidb::sql::IntegerUnaryOperator;
using unidb::sql::SqlValue;
using unidb::sql::TextFunction;

namespace {
unidb::sql::SqlNumericValue asNumeric(const SqlValue &value) {
  if (const auto *numeric = std::get_if<unidb::sql::SqlNumericValue>(&value)) {
    return *numeric;
  }
  if (const auto *integer = std::get_if<std::int64_t>(&value)) {
    return parseSqlNumericLiteral(std::to_string(*integer));
  }
  throw std::invalid_argument("Numeric arithmetic requires numeric operands");
}
} // namespace

SqlValue evaluateTextFunction(const std::vector<SqlValue> &row,
                              const CompiledTextFunction &expression) {
  const auto &value = std::get<std::string>(row.at(expression.columnIndex));
  if (expression.function == TextFunction::Length) {
    if (value.size() >
        static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
      throw std::invalid_argument("LENGTH result exceeds INT64 range");
    }
    return static_cast<std::int64_t>(value.size());
  }

  std::string result = value;
  for (char &character : result) {
    const auto byte = static_cast<unsigned char>(character);
    if (expression.function == TextFunction::Upper && byte >= 'a' &&
        byte <= 'z') {
      character = static_cast<char>(byte - ('a' - 'A'));
    } else if (expression.function == TextFunction::Lower && byte >= 'A' &&
               byte <= 'Z') {
      character = static_cast<char>(byte + ('a' - 'A'));
    }
  }
  return result;
}

SqlValue
evaluateSqlArithmeticExpression(const std::vector<SqlValue> &row,
                                const CompiledIntegerExpression &expression) {
  if (expression.kind == IntegerExpression::Kind::Literal) {
    if (expression.numericLiteral.has_value()) {
      return *expression.numericLiteral;
    }
    return expression.literal;
  }
  if (expression.kind == IntegerExpression::Kind::Column) {
    return row.at(expression.columnIndex);
  }
  if (expression.kind == IntegerExpression::Kind::Unary) {
    if (!expression.operand) {
      throw std::invalid_argument("Malformed unary integer expression");
    }
    const auto operand =
        evaluateSqlArithmeticExpression(row, *expression.operand);
    if (expression.numeric) {
      auto numeric = asNumeric(operand);
      if (expression.unaryOperation == IntegerUnaryOperator::Negative &&
          numeric.coefficient != "0") {
        numeric.negative = !numeric.negative;
      }
      return numeric;
    }
    const auto integer = std::get<std::int64_t>(operand);
    if (expression.unaryOperation == IntegerUnaryOperator::Positive) {
      return integer;
    }
    if (integer == std::numeric_limits<std::int64_t>::min()) {
      throw std::invalid_argument("Integer negation exceeds INT64 range");
    }
    return -integer;
  }
  if (!expression.left || !expression.right) {
    throw std::invalid_argument("Malformed integer arithmetic expression");
  }
  const auto left = evaluateSqlArithmeticExpression(row, *expression.left);
  const auto right = evaluateSqlArithmeticExpression(row, *expression.right);
  if (expression.numeric) {
    const auto numericLeft = asNumeric(left);
    const auto numericRight = asNumeric(right);
    switch (expression.operation) {
    case IntegerBinaryOperator::Add:
      return addSqlNumericValues(numericLeft, numericRight);
    case IntegerBinaryOperator::Subtract:
      return subtractSqlNumericValues(numericLeft, numericRight);
    case IntegerBinaryOperator::Multiply:
      return multiplySqlNumericValues(numericLeft, numericRight);
    case IntegerBinaryOperator::Divide: {
      // UniDB SQL numeric division currently returns at least six fractional
      // digits, preserving any greater input scale.
      const auto resultScale = std::max<std::int32_t>(
          6, std::max(numericLeft.scale, numericRight.scale));
      return divideSqlNumericValues(numericLeft, numericRight, resultScale);
    }
    }
    throw std::invalid_argument("Unknown numeric arithmetic operator");
  }
  const auto integerLeft = std::get<std::int64_t>(left);
  const auto integerRight = std::get<std::int64_t>(right);
  std::int64_t result{};
  switch (expression.operation) {
  case IntegerBinaryOperator::Add:
    if (__builtin_add_overflow(integerLeft, integerRight, &result)) {
      throw std::invalid_argument("Integer addition exceeds INT64 range");
    }
    return result;
  case IntegerBinaryOperator::Subtract:
    if (__builtin_sub_overflow(integerLeft, integerRight, &result)) {
      throw std::invalid_argument("Integer subtraction exceeds INT64 range");
    }
    return result;
  case IntegerBinaryOperator::Multiply:
    if (__builtin_mul_overflow(integerLeft, integerRight, &result)) {
      throw std::invalid_argument("Integer multiplication exceeds INT64 range");
    }
    return result;
  case IntegerBinaryOperator::Divide:
    if (integerRight == 0) {
      throw std::invalid_argument("Division by zero in integer expression");
    }
    if (integerLeft == std::numeric_limits<std::int64_t>::min() &&
        integerRight == -1) {
      throw std::invalid_argument("Integer division exceeds INT64 range");
    }
    return integerLeft / integerRight;
  }
  throw std::invalid_argument("Unknown integer arithmetic operator");
}

std::int64_t
evaluateIntegerExpression(const std::vector<SqlValue> &row,
                          const CompiledIntegerExpression &expression) {
  return std::get<std::int64_t>(
      evaluateSqlArithmeticExpression(row, expression));
}

} // namespace QueryExecutionService::domain
