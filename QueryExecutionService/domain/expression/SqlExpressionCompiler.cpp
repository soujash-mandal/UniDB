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

    std::vector<std::size_t> selectedIndexes;
    std::vector<ColumnDefinition> selectedColumns;
    std::vector<std::optional<std::vector<CompiledWhereExpression>>>
        compiledCases;
    std::vector<std::optional<CompiledIntegerExpression>>
        compiledIntegers;
    std::vector<std::optional<CompiledTextFunction>>
        compiledTextFunctions;
    if (select->aggregate.has_value() && select->groupBy.empty()) {
      // Ungrouped aggregate queries produce a single computed output column.
    } else if (select->selectAll) {
      selectedIndexes.reserve(schema.size());
      selectedColumns = schema;
      for (std::size_t index = 0; index < schema.size(); ++index) {
        selectedIndexes.push_back(index);
      }
    } else {
      if (select->columnNames.empty()) {
        throw std::invalid_argument("SELECT requires at least one column");
      }
      selectedIndexes.reserve(select->columnNames.size());
      selectedColumns.reserve(select->columnNames.size());
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      if (!select->caseExpressions.empty() &&
          select->caseExpressions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT CASE expression count does not match columns");
      }
      if (!select->integerExpressions.empty() &&
          select->integerExpressions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT integer expression count does not match columns");
      }
      if (!select->textFunctions.empty() &&
          select->textFunctions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT text function count does not match columns");
      }
      compiledCases.reserve(select->columnNames.size());
      compiledIntegers.reserve(select->columnNames.size());
      compiledTextFunctions.reserve(select->columnNames.size());
      for (std::size_t itemIndex = 0; itemIndex < select->columnNames.size();
           ++itemIndex) {
        if (itemIndex < select->textFunctions.size() &&
            select->textFunctions[itemIndex].has_value()) {
          if (!select->innerJoins.empty()) {
            throw std::invalid_argument(
                "Scalar text functions are not yet supported with JOIN");
          }
          const auto &expression = *select->textFunctions[itemIndex];
          const auto compiled =
              compileTextFunction(expression, schema, types, tableQualifier);
          selectedIndexes.push_back(schema.size());
          const auto alias = itemIndex < select->columnAliases.size()
                                 ? select->columnAliases[itemIndex]
                                 : std::optional<std::string>{}

    std::vector<std::size_t> selectedIndexes;
    std::vector<ColumnDefinition> selectedColumns;
    std::vector<std::optional<std::vector<domain::CompiledWhereExpression>>>
        compiledCases;
    std::vector<std::optional<domain::CompiledIntegerExpression>>
        compiledIntegers;
    std::vector<std::optional<domain::CompiledTextFunction>>
        compiledTextFunctions;
    if (select->aggregate.has_value() && select->groupBy.empty()) {
      // Ungrouped aggregate queries produce a single computed output column.
    } else if (select->selectAll) {
      selectedIndexes.reserve(schema.size());
      selectedColumns = schema;
      for (std::size_t index = 0; index < schema.size(); ++index) {
        selectedIndexes.push_back(index);
      }
    } else {
      if (select->columnNames.empty()) {
        throw std::invalid_argument("SELECT requires at least one column");
      }
      selectedIndexes.reserve(select->columnNames.size());
      selectedColumns.reserve(select->columnNames.size());
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      if (!select->caseExpressions.empty() &&
          select->caseExpressions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT CASE expression count does not match columns");
      }
      if (!select->integerExpressions.empty() &&
          select->integerExpressions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT integer expression count does not match columns");
      }
      if (!select->textFunctions.empty() &&
          select->textFunctions.size() != select->columnNames.size()) {
        throw std::invalid_argument(
            "SELECT text function count does not match columns");
      }
      compiledCases.reserve(select->columnNames.size());
      compiledIntegers.reserve(select->columnNames.size());
      compiledTextFunctions.reserve(select->columnNames.size());
      for (std::size_t itemIndex = 0; itemIndex < select->columnNames.size();
           ++itemIndex) {
        if (itemIndex < select->textFunctions.size() &&
            select->textFunctions[itemIndex].has_value()) {
          if (!select->innerJoins.empty()) {
            throw std::invalid_argument(
                "Scalar text functions are not yet supported with JOIN");
          }
          const auto &expression = *select->textFunctions[itemIndex];
          const auto compiled =
              domain::compileTextFunction(expression, schema, types, tableQualifier);
          selectedIndexes.push_back(schema.size());
          const auto alias = itemIndex < select->columnAliases.size()
                                 ? select->columnAliases[itemIndex]
                                 : std::optional<std::string>{};
          const auto functionName = domain::textFunctionName(expression.function);
          const auto outputType = expression.function == TextFunction::Length
                                      ? SqlType::Int
                                      : SqlType::Text;
          selectedColumns.push_back(
              {alias.value_or(functionName + "(" + expression.columnName + ")"),
               outputType}

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
