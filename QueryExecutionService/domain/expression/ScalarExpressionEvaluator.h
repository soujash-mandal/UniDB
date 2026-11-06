#pragma once

#include "../contract/sqlStatement.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace QueryExecutionService::domain {

struct CompiledTextFunction {
  unidb::sql::TextFunction function{unidb::sql::TextFunction::Upper};
  std::size_t columnIndex{0};
};

struct CompiledIntegerExpression {
  unidb::sql::IntegerExpression::Kind kind{
      unidb::sql::IntegerExpression::Kind::Literal};
  std::int64_t literal{0};
  std::size_t columnIndex{0};
  unidb::sql::IntegerBinaryOperator operation{
      unidb::sql::IntegerBinaryOperator::Add};
  unidb::sql::IntegerUnaryOperator unaryOperation{
      unidb::sql::IntegerUnaryOperator::Positive};
  std::shared_ptr<CompiledIntegerExpression> operand;
  std::shared_ptr<CompiledIntegerExpression> left;
  std::shared_ptr<CompiledIntegerExpression> right;
};

unidb::sql::SqlValue
evaluateTextFunction(const std::vector<unidb::sql::SqlValue> &row,
                     const CompiledTextFunction &expression);

std::int64_t
evaluateIntegerExpression(const std::vector<unidb::sql::SqlValue> &row,
                          const CompiledIntegerExpression &expression);

} // namespace QueryExecutionService::domain
