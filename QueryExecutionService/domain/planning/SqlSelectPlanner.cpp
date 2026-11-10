#include "SqlSelectPlanner.h"

#include "../expression/SqlExpressionCompiler.h"
#include "../predicate/WhereExpressionNormalizer.h"
#include "../schema/SqlSchemaResolver.h"
#include "../types/SqlNumericSemantics.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace QueryExecutionService::domain {

using namespace unidb::sql;

SelectPlan
buildSelectPlan(const unidb::sql::SelectStatement &select,
                const std::vector<unidb::sql::ColumnDefinition> &schema) {
  SelectPlan plan;
  plan.schema = schema;
  plan.types = schemaTypes(schema);
  const auto &types = plan.types;
  const auto *where = std::get_if<WhereExpression>(&select.where);
  if (where != nullptr) {
    const std::string tableQualifier =
        select.tableAlias.value_or(select.tableName);
    const auto normalized =
        normalizeSingleTableWhereColumns(*where, tableQualifier);
    plan.compiledWhere = compileWhereExpression(normalized, schema, types);
  }

  if (select.aggregate.has_value() && select.groupBy.empty()) {
    // Ungrouped aggregate queries produce a single computed output column.
  } else if (select.selectAll) {
    plan.selectedIndexes.reserve(schema.size());
    plan.selectedColumns = schema;
    for (std::size_t index = 0; index < schema.size(); ++index) {
      plan.selectedIndexes.push_back(index);
    }
  } else {
    if (select.columnNames.empty()) {
      throw std::invalid_argument("SELECT requires at least one column");
    }
    plan.selectedIndexes.reserve(select.columnNames.size());
    plan.selectedColumns.reserve(select.columnNames.size());
    const std::string tableQualifier =
        select.tableAlias.value_or(select.tableName);
    if (!select.caseExpressions.empty() &&
        select.caseExpressions.size() != select.columnNames.size()) {
      throw std::invalid_argument(
          "SELECT CASE expression count does not match columns");
    }
    if (!select.integerExpressions.empty() &&
        select.integerExpressions.size() != select.columnNames.size()) {
      throw std::invalid_argument(
          "SELECT integer expression count does not match columns");
    }
    if (!select.textFunctions.empty() &&
        select.textFunctions.size() != select.columnNames.size()) {
      throw std::invalid_argument(
          "SELECT text function count does not match columns");
    }
    plan.compiledCases.reserve(select.columnNames.size());
    plan.compiledIntegers.reserve(select.columnNames.size());
    plan.compiledTextFunctions.reserve(select.columnNames.size());
    for (std::size_t itemIndex = 0; itemIndex < select.columnNames.size();
         ++itemIndex) {
      if (itemIndex < select.textFunctions.size() &&
          select.textFunctions[itemIndex].has_value()) {
        if (!select.innerJoins.empty()) {
          throw std::invalid_argument(
              "Scalar text functions are not yet supported with JOIN");
        }
        const auto &expression = *select.textFunctions[itemIndex];
        const auto compiled =
            compileTextFunction(expression, schema, types, tableQualifier);
        plan.selectedIndexes.push_back(schema.size());
        const auto alias = itemIndex < select.columnAliases.size()
                               ? select.columnAliases[itemIndex]
                               : std::optional<std::string>{};
        const auto functionName = textFunctionName(expression.function);
        const auto outputType = expression.function == TextFunction::Length
                                    ? SqlType::Int
                                    : SqlType::Text;
        plan.selectedColumns.push_back(
            {alias.value_or(functionName + "(" + expression.columnName + ")"),
             outputType});
        plan.compiledCases.push_back(std::nullopt);
        plan.compiledIntegers.push_back(std::nullopt);
        plan.compiledTextFunctions.push_back(compiled);
        continue;
      }
      if (itemIndex < select.integerExpressions.size() &&
          select.integerExpressions[itemIndex].has_value()) {
        if (!select.innerJoins.empty()) {
          throw std::invalid_argument(
              "Integer expressions are not yet supported with JOIN");
        }
        const auto compiled =
            compileIntegerExpression(*select.integerExpressions[itemIndex],
                                     schema, types, tableQualifier);
        plan.selectedIndexes.push_back(schema.size());
        const auto alias = itemIndex < select.columnAliases.size()
                               ? select.columnAliases[itemIndex]
                               : std::optional<std::string>{};
        std::optional<SqlTypeDescriptor> expressionType;
        if (compiled.numeric) {
          expressionType = SqlTypeDescriptor{SqlTypeId::Numeric, {}};
        }
        plan.selectedColumns.push_back(
            {alias.value_or("EXPR" + std::to_string(itemIndex + 1)),
             SqlType::Int, expressionType});
        plan.compiledCases.push_back(std::nullopt);
        plan.compiledIntegers.push_back(compiled);
        plan.compiledTextFunctions.push_back(std::nullopt);
        continue;
      }
      if (itemIndex < select.caseExpressions.size() &&
          select.caseExpressions[itemIndex].has_value()) {
        if (!select.innerJoins.empty()) {
          throw std::invalid_argument(
              "CASE expressions are not yet supported with JOIN");
        }
        const auto &caseExpression = *select.caseExpressions[itemIndex];
        const SqlValue &elseValue = caseExpression.elseResult;
        const bool resultIsInt =
            std::holds_alternative<std::int64_t>(elseValue);
        std::vector<CompiledWhereExpression> branches;
        branches.reserve(caseExpression.branches.size());
        for (const auto &branch : caseExpression.branches) {
          if (std::holds_alternative<std::int64_t>(branch.result) !=
              resultIsInt) {
            throw std::invalid_argument(
                "CASE result values must have the same type");
          }
          const auto normalized = normalizeSingleTableWhereColumns(
              branch.condition, tableQualifier);
          branches.push_back(compileWhereExpression(normalized, schema, types));
        }
        plan.selectedIndexes.push_back(schema.size());
        plan.selectedColumns.push_back(
            {select.columnAliases[itemIndex].value_or("CASE"),
             resultIsInt ? SqlType::Int : SqlType::Text});
        plan.compiledCases.push_back(std::move(branches));
        plan.compiledIntegers.push_back(std::nullopt);
        plan.compiledTextFunctions.push_back(std::nullopt);
        continue;
      }
      const auto &columnName = select.columnNames[itemIndex];
      const auto normalizedColumnName =
          normalizeSingleTableColumnName(columnName, tableQualifier);
      const auto found =
          std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == normalizedColumnName;
          });
      if (found == schema.end()) {
        throw std::invalid_argument("Unknown SELECT column: " + columnName);
      }
      const auto schemaIndex =
          static_cast<std::size_t>(std::distance(schema.begin(), found));
      plan.selectedIndexes.push_back(schemaIndex);
      plan.selectedColumns.push_back(*found);
      plan.compiledCases.push_back(std::nullopt);
      plan.compiledIntegers.push_back(std::nullopt);
      plan.compiledTextFunctions.push_back(std::nullopt);
    }

    if (!select.columnAliases.empty() &&
        select.columnAliases.size() != select.columnNames.size()) {
      throw std::invalid_argument("SELECT alias count does not match columns");
    }
    std::unordered_set<std::string> outputNames;
    for (std::size_t index = 0; index < plan.selectedColumns.size(); ++index) {
      if (index < select.columnAliases.size() &&
          select.columnAliases[index].has_value()) {
        plan.selectedColumns[index].name = *select.columnAliases[index];
      } else if (index < select.caseExpressions.size() &&
                 select.caseExpressions[index].has_value()) {
        plan.selectedColumns[index].name = "CASE";
      }
      if (!outputNames.insert(plan.selectedColumns[index].name).second) {
        throw std::invalid_argument("Duplicate SELECT output column name: " +
                                    plan.selectedColumns[index].name);
      }
    }
  }

  const bool hasCaseExpression = std::any_of(
      select.caseExpressions.begin(), select.caseExpressions.end(),
      [](const auto &expression) { return expression.has_value(); });
  const bool hasTextFunction = std::any_of(
      select.textFunctions.begin(), select.textFunctions.end(),
      [](const auto &expression) { return expression.has_value(); });
  if (hasTextFunction && select.distinct) {
    throw std::invalid_argument(
        "SELECT DISTINCT cannot be combined with scalar text functions yet");
  }
  if (hasCaseExpression && select.distinct) {
    throw std::invalid_argument(
        "SELECT DISTINCT cannot be combined with CASE expressions yet");
  }

  plan.orderIndexes.reserve(select.orderBy.size());
  const std::string tableQualifier =
      select.tableAlias.value_or(select.tableName);
  if (select.groupBy.empty()) {
    for (const auto &order : select.orderBy) {
      const auto alias = std::find_if(
          plan.selectedColumns.begin(), plan.selectedColumns.end(),
          [&](const auto &column) { return column.name == order.columnName; });
      if (alias != plan.selectedColumns.end()) {
        const auto projectionIndex = static_cast<std::size_t>(
            std::distance(plan.selectedColumns.begin(), alias));
        plan.orderIndexes.push_back(
            {projectionIndex, order.ascending, true, alias->declaredType});
        continue;
      }

      const auto normalizedOrderColumn =
          normalizeSingleTableColumnName(order.columnName, tableQualifier);
      const auto found =
          std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == normalizedOrderColumn;
          });
      if (found == schema.end()) {
        throw std::invalid_argument("Unknown ORDER BY column: " +
                                    order.columnName);
      }
      const auto orderIndex =
          static_cast<std::size_t>(std::distance(schema.begin(), found));
      if (select.distinct &&
          std::find(plan.selectedIndexes.begin(), plan.selectedIndexes.end(),
                    orderIndex) == plan.selectedIndexes.end()) {
        throw std::invalid_argument("ORDER BY column must appear in SELECT "
                                    "list when DISTINCT is used: " +
                                    order.columnName);
      }
      plan.orderIndexes.push_back(
          {orderIndex, order.ascending, false, found->declaredType});
    }
  }

  if (select.having.has_value() && select.groupBy.empty()) {
    throw std::invalid_argument("HAVING requires GROUP BY");
  }
  if (!select.groupBy.empty()) {
    if (!select.aggregate.has_value()) {
      throw std::invalid_argument(
          "GROUP BY requires one aggregate expression in SELECT");
    }
    if (select.columnNames != select.groupBy) {
      throw std::invalid_argument(
          "SELECT columns must exactly match GROUP BY columns");
    }
    for (const auto &groupColumn : select.groupBy) {
      const auto normalizedGroupColumn =
          normalizeSingleTableColumnName(groupColumn, tableQualifier);
      const auto found =
          std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == normalizedGroupColumn;
          });
      if (found == schema.end()) {
        throw std::invalid_argument("Unknown GROUP BY column: " + groupColumn);
      }
      plan.groupIndexes.push_back(
          static_cast<std::size_t>(std::distance(schema.begin(), found)));
    }
  }
  if (select.aggregate.has_value()) {
    const auto &aggregate = *select.aggregate;
    if ((select.groupBy.empty() && select.distinct) ||
        (select.groupBy.empty() &&
         (!select.orderBy.empty() || select.limit.has_value() ||
          select.offset != 0))) {
      throw std::invalid_argument(
          "Aggregate SELECT cannot be combined with DISTINCT, ORDER BY, "
          "LIMIT, or OFFSET");
    }
    if (aggregate.columnName.has_value()) {
      const auto found =
          std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == *aggregate.columnName;
          });
      if (found == schema.end()) {
        throw std::invalid_argument("Unknown aggregate column: " +
                                    *aggregate.columnName);
      }
      plan.aggregateIndex =
          static_cast<std::size_t>(std::distance(schema.begin(), found));
    } else if (aggregate.function != AggregateFunction::Count) {
      throw std::invalid_argument("Only COUNT supports '*'");
    }
    if ((aggregate.function == AggregateFunction::Sum ||
         aggregate.function == AggregateFunction::Avg) &&
        (!plan.aggregateIndex.has_value() ||
         types[*plan.aggregateIndex] != SqlType::Int)) {
      throw std::invalid_argument("SUM and AVG require an INT column");
    }
    if (!select.groupBy.empty() && select.having.has_value()) {
      const auto &having = *select.having;
      if (having.aggregate.function != aggregate.function ||
          having.aggregate.columnName != aggregate.columnName) {
        throw std::invalid_argument(
            "HAVING aggregate must match the SELECT aggregate");
      }
      const bool numericAggregate =
          aggregate.function != AggregateFunction::Count &&
          plan.aggregateIndex.has_value() &&
          plan.schema[*plan.aggregateIndex].declaredType.has_value() &&
          plan.schema[*plan.aggregateIndex].declaredType->id ==
              SqlTypeId::Numeric;
      if (numericAggregate) {
        if (const auto *integer = std::get_if<std::int64_t>(&having.value)) {
          plan.havingValue = parseSqlNumericLiteral(std::to_string(*integer));
        } else if (std::holds_alternative<SqlNumericValue>(having.value)) {
          plan.havingValue = having.value;
        } else {
          throw std::invalid_argument(
              "HAVING value type does not match aggregate result type");
        }
      } else {
        const SqlType resultType =
            aggregate.function == AggregateFunction::Min ||
                    aggregate.function == AggregateFunction::Max
                ? types[*plan.aggregateIndex]
                : SqlType::Int;
        const bool valueIsInt =
            std::holds_alternative<std::int64_t>(having.value);
        if ((resultType == SqlType::Int) != valueIsInt) {
          throw std::invalid_argument(
              "HAVING value type does not match aggregate result type");
        }
        plan.havingValue = having.value;
      }
    }
  }

  if (!select.groupBy.empty()) {
    const auto &aggregate = *select.aggregate;
    const std::string functionName =
        aggregate.function == AggregateFunction::Count ? "COUNT"
        : aggregate.function == AggregateFunction::Min ? "MIN"
        : aggregate.function == AggregateFunction::Max ? "MAX"
        : aggregate.function == AggregateFunction::Sum ? "SUM"
                                                       : "AVG";
    const std::string aggregateName = aggregate.alias.value_or(
        functionName + "(" +
        (aggregate.columnName.has_value() ? *aggregate.columnName : "*") + ")");
    plan.groupedOrderIndexes.reserve(select.orderBy.size());
    for (const auto &order : select.orderBy) {
      const auto projected = std::find_if(
          plan.selectedColumns.begin(), plan.selectedColumns.end(),
          [&](const auto &column) { return column.name == order.columnName; });
      if (projected != plan.selectedColumns.end()) {
        plan.groupedOrderIndexes.push_back(
            {static_cast<std::size_t>(
                 std::distance(plan.selectedColumns.begin(), projected)),
             order.ascending, projected->declaredType});
        continue;
      }
      if (order.columnName == aggregateName) {
        const auto aggregateType =
            (aggregate.function == AggregateFunction::Min ||
             aggregate.function == AggregateFunction::Max) &&
                    plan.aggregateIndex.has_value()
                ? plan.schema[*plan.aggregateIndex].declaredType
                : std::optional<SqlTypeDescriptor>{};
        plan.groupedOrderIndexes.push_back(
            {plan.selectedColumns.size(), order.ascending, aggregateType});
        continue;
      }

      const auto normalizedOrderColumn =
          normalizeSingleTableColumnName(order.columnName, tableQualifier);
      const auto groupIndex =
          std::find_if(plan.groupIndexes.begin(), plan.groupIndexes.end(),
                       [&](const auto index) {
                         return schema[index].name == normalizedOrderColumn;
                       });
      if (groupIndex == plan.groupIndexes.end()) {
        throw std::invalid_argument("Unknown GROUP BY ORDER BY column: " +
                                    order.columnName);
      }
      plan.groupedOrderIndexes.push_back(
          {static_cast<std::size_t>(
               std::distance(plan.groupIndexes.begin(), groupIndex)),
           order.ascending, schema[*groupIndex].declaredType});
    }
  }
  return plan;
}

} // namespace QueryExecutionService::domain
