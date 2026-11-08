#include "SqlExpressionCompiler.h"

#include "../schema/SqlSchemaResolver.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace QueryExecutionService::domain {
using unidb::sql::ColumnDefinition;
using unidb::sql::IntegerExpression;
using unidb::sql::SqlType;
using unidb::sql::TextFunction;
using unidb::sql::TextFunctionExpression;

CompiledTextFunction
compileTextFunction(const TextFunctionExpression &expression,
                    const std::vector<ColumnDefinition> &schema,
                    const std::vector<SqlType> &types,
                    const std::string &tableQualifier) {
  const auto name =
      normalizeSingleTableColumnName(expression.columnName, tableQualifier);
  const auto found =
      std::find_if(schema.begin(), schema.end(),
                   [&](const auto &column) { return column.name == name; });
  if (found == schema.end()) {
    throw std::invalid_argument("Unknown SELECT column: " +
                                expression.columnName);
  }
  const auto index =
      static_cast<std::size_t>(std::distance(schema.begin(), found));
  if (types[index] != SqlType::Text) {
    throw std::invalid_argument("Scalar text functions require TEXT columns: " +
                                expression.columnName);
  }
  return {expression.function, index};
}

std::string textFunctionName(TextFunction function) {
  switch (function) {
  case TextFunction::Upper:
    return "UPPER";
  case TextFunction::Lower:
    return "LOWER";
  case TextFunction::Length:
    return "LENGTH";
  }
  throw std::invalid_argument("Unknown scalar text function");
}

CompiledIntegerExpression
compileIntegerExpression(const IntegerExpression &expression,
                         const std::vector<ColumnDefinition> &schema,
                         const std::vector<SqlType> &types,
                         const std::string &tableQualifier) {
  CompiledIntegerExpression compiled;
  compiled.kind = expression.kind;
  if (expression.kind == IntegerExpression::Kind::Literal) {
    compiled.literal = expression.literal;
    return compiled;
  }
  if (expression.kind == IntegerExpression::Kind::Column) {
    const auto name =
        normalizeSingleTableColumnName(expression.columnName, tableQualifier);
    const auto found =
        std::find_if(schema.begin(), schema.end(),
                     [&](const auto &column) { return column.name == name; });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown SELECT column: " +
                                  expression.columnName);
    }
    const auto index =
        static_cast<std::size_t>(std::distance(schema.begin(), found));
    if (types[index] != SqlType::Int) {
      throw std::invalid_argument("Integer arithmetic requires INT columns: " +
                                  expression.columnName);
    }
    compiled.columnIndex = index;
    return compiled;
  }
  if (expression.kind == IntegerExpression::Kind::Unary) {
    if (!expression.operand) {
      throw std::invalid_argument("Malformed unary integer expression");
    }
    compiled.unaryOperation = expression.unaryOperation;
    compiled.operand = std::make_shared<CompiledIntegerExpression>(
        compileIntegerExpression(*expression.operand, schema, types,
                                 tableQualifier));
    return compiled;
  }
  if (!expression.left || !expression.right) {
    throw std::invalid_argument("Malformed integer arithmetic expression");
  }
  compiled.operation = expression.operation;
  compiled.left = std::make_shared<CompiledIntegerExpression>(
      compileIntegerExpression(*expression.left, schema, types,
                               tableQualifier));
  compiled.right = std::make_shared<CompiledIntegerExpression>(
      compileIntegerExpression(*expression.right, schema, types,
                               tableQualifier));
  return compiled;
}

} // namespace QueryExecutionService::domain
