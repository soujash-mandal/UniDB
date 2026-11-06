#include "../../domain/expression/ScalarExpressionEvaluator.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
using QueryExecutionService::domain::CompiledIntegerExpression;
using QueryExecutionService::domain::CompiledTextFunction;
using QueryExecutionService::domain::evaluateIntegerExpression;
using QueryExecutionService::domain::evaluateTextFunction;
using unidb::sql::IntegerBinaryOperator;
using unidb::sql::IntegerExpression;
using unidb::sql::IntegerUnaryOperator;
using unidb::sql::SqlValue;
using unidb::sql::TextFunction;
} // namespace

TEST(ScalarExpressionEvaluatorTest, EvaluatesTextFunctions) {
  const std::vector<SqlValue> row{std::string("Abc"), std::string("hello")};
  EXPECT_EQ(std::get<std::string>(evaluateTextFunction(
                row, CompiledTextFunction{TextFunction::Upper, 0})),
            "ABC");
  EXPECT_EQ(std::get<std::string>(evaluateTextFunction(
                row, CompiledTextFunction{TextFunction::Lower, 0})),
            "abc");
  EXPECT_EQ(std::get<std::int64_t>(evaluateTextFunction(
                row, CompiledTextFunction{TextFunction::Length, 1})),
            5);
}

TEST(ScalarExpressionEvaluatorTest,
     EvaluatesIntegerLiteralsColumnsAndOperators) {
  const std::vector<SqlValue> row{std::int64_t{7}};
  CompiledIntegerExpression literal;
  literal.literal = 12;
  EXPECT_EQ(evaluateIntegerExpression(row, literal), 12);

  CompiledIntegerExpression column;
  column.kind = IntegerExpression::Kind::Column;
  column.columnIndex = 0;
  EXPECT_EQ(evaluateIntegerExpression(row, column), 7);

  CompiledIntegerExpression left;
  left.literal = 9;
  CompiledIntegerExpression right;
  right.literal = 3;
  CompiledIntegerExpression binary;
  binary.kind = IntegerExpression::Kind::Binary;
  binary.operation = IntegerBinaryOperator::Multiply;
  binary.left = std::make_shared<CompiledIntegerExpression>(left);
  binary.right = std::make_shared<CompiledIntegerExpression>(right);
  EXPECT_EQ(evaluateIntegerExpression(row, binary), 27);

  CompiledIntegerExpression unary;
  unary.kind = IntegerExpression::Kind::Unary;
  unary.unaryOperation = IntegerUnaryOperator::Negative;
  unary.operand = std::make_shared<CompiledIntegerExpression>(column);
  EXPECT_EQ(evaluateIntegerExpression(row, unary), -7);
}

TEST(ScalarExpressionEvaluatorTest, RejectsDivisionByZeroAndOverflow) {
  const std::vector<SqlValue> row;
  CompiledIntegerExpression left;
  left.literal = std::numeric_limits<std::int64_t>::min();
  CompiledIntegerExpression minusOne;
  minusOne.literal = -1;
  CompiledIntegerExpression divide;
  divide.kind = IntegerExpression::Kind::Binary;
  divide.operation = IntegerBinaryOperator::Divide;
  divide.left = std::make_shared<CompiledIntegerExpression>(left);
  divide.right = std::make_shared<CompiledIntegerExpression>(minusOne);
  EXPECT_THROW(evaluateIntegerExpression(row, divide), std::invalid_argument);

  CompiledIntegerExpression zero;
  zero.literal = 0;
  divide.right = std::make_shared<CompiledIntegerExpression>(zero);
  EXPECT_THROW(evaluateIntegerExpression(row, divide), std::invalid_argument);
}

TEST(ScalarExpressionEvaluatorTest, RejectsNegationOfInt64Minimum) {
  const std::vector<SqlValue> row;
  CompiledIntegerExpression minimum;
  minimum.literal = std::numeric_limits<std::int64_t>::min();
  CompiledIntegerExpression unary;
  unary.kind = IntegerExpression::Kind::Unary;
  unary.unaryOperation = IntegerUnaryOperator::Negative;
  unary.operand = std::make_shared<CompiledIntegerExpression>(minimum);
  EXPECT_THROW(evaluateIntegerExpression(row, unary), std::invalid_argument);
}
} // namespace
