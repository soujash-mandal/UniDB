#include "ScalarExpressionEvaluator.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace QueryExecutionService::domain {
using unidb::sql::IntegerBinaryOperator;
using unidb::sql::IntegerExpression;
using unidb::sql::IntegerUnaryOperator;
using unidb::sql::SqlValue;
using unidb::sql::TextFunction;

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

std::int64_t
evaluateIntegerExpression(const std::vector<SqlValue> &row,
                          const CompiledIntegerExpression &expression) {
  if (expression.kind == IntegerExpression::Kind::Literal) {
    return expression.literal;
  }
  if (expression.kind == IntegerExpression::Kind::Column) {
    return std::get<std::int64_t>(row.at(expression.columnIndex));
  }
  if (expression.kind == IntegerExpression::Kind::Unary) {
    if (!expression.operand) {
      throw std::invalid_argument("Malformed unary integer expression");
    }
    const auto operand = evaluateIntegerExpression(row, *expression.operand);
    if (expression.unaryOperation == IntegerUnaryOperator::Positive) {
      return operand;
    }
    if (operand == std::numeric_limits<std::int64_t>::min()) {
      throw std::invalid_argument("Integer negation exceeds INT64 range");
    }
    return -operand;
  }
  const auto left = evaluateIntegerExpression(row, *expression.left);
  const auto right = evaluateIntegerExpression(row, *expression.right);
  std::int64_t result{};
  switch (expression.operation) {
  case IntegerBinaryOperator::Add:
    if (__builtin_add_overflow(left, right, &result)) {
      throw std::invalid_argument("Integer addition exceeds INT64 range");
    }
    return result;
  case IntegerBinaryOperator::Subtract:
    if (__builtin_sub_overflow(left, right, &result)) {
      throw std::invalid_argument("Integer subtraction exceeds INT64 range");
    }
    return result;
  case IntegerBinaryOperator::Multiply:
    if (__builtin_mul_overflow(left, right, &result)) {
      throw std::invalid_argument("Integer multiplication exceeds INT64 range");
    }
    return result;
  case IntegerBinaryOperator::Divide:
    if (right == 0) {
      throw std::invalid_argument("Division by zero in integer expression");
    }
    if (left == std::numeric_limits<std::int64_t>::min() && right == -1) {
      throw std::invalid_argument("Integer division exceeds INT64 range");
    }
    return left / right;
  }
  throw std::invalid_argument("Unknown integer arithmetic operator");
}

} // namespace QueryExecutionService::domain
