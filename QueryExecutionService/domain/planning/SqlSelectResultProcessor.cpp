#include "SqlSelectResultProcessor.h"

#include "../aggregate/SqlAggregateCalculator.h"
#include "../codec/SqlTupleCodec.h"
#include "../predicate/WherePredicateEvaluator.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace QueryExecutionService::domain {

using namespace unidb::sql;
std::vector<std::vector<SqlValue>> decodeAndFilterRows(
    const std::vector<std::vector<char>> &encodedRows,
    const std::vector<SqlType> &types,
    const std::optional<CompiledWhereExpression> &compiledWhere) {
  std::vector<std::vector<SqlValue>> matchedRows;
  matchedRows.reserve(encodedRows.size());
  for (const auto &bytes : encodedRows) {
    auto decoded = SqlTupleCodec::decode(types, bytes);
    if (compiledWhere.has_value() &&
        !matchesWhereExpression(decoded, *compiledWhere)) {
      continue;
    }
    matchedRows.push_back(std::move(decoded));
  }
  return matchedRows;
}



QueryExecutionService::QueryExecutionResult
processSelectRows(const unidb::sql::SelectStatement &select,
                  const SelectPlan &plan,
                  std::vector<std::vector<unidb::sql::SqlValue>> matchedRows) {
  using namespace unidb::sql;
  if (!select.groupBy.empty()) {
    const auto &aggregate = *select.aggregate;
    const std::string functionName =
        aggregate.function == AggregateFunction::Count ? "COUNT"
        : aggregate.function == AggregateFunction::Min ? "MIN"
        : aggregate.function == AggregateFunction::Max ? "MAX"
        : aggregate.function == AggregateFunction::Sum ? "SUM"
                                                       : "AVG";
    const SqlType aggregateType =
        aggregate.function == AggregateFunction::Min ||
                aggregate.function == AggregateFunction::Max
            ? plan.types[*plan.aggregateIndex]
            : SqlType::Int;
    using GroupRows = std::map<std::vector<SqlValue>,
                               std::vector<const std::vector<SqlValue> *>>;
    GroupRows groups;
    for (const auto &row : matchedRows) {
      std::vector<SqlValue> key;
      key.reserve(plan.groupIndexes.size());
      for (const auto index : plan.groupIndexes) {
        key.push_back(row[index]);
      }
      groups[std::move(key)].push_back(&row);
    }

    QueryExecutionResult result;
    result.kind = QueryExecutionResultKind::Rows;
    result.message = "Rows selected";
    result.columns = plan.selectedColumns;
    const std::string aggregateName = aggregate.alias.value_or(
        functionName + "(" +
        (aggregate.columnName.has_value() ? *aggregate.columnName : "*") + ")");
    if (std::any_of(
            result.columns.begin(), result.columns.end(),
            [&](const auto &column) { return column.name == aggregateName; })) {
      throw std::invalid_argument("Duplicate SELECT output column name: " +
                                  aggregateName);
    }
    result.columns.push_back({aggregateName, aggregateType});

    for (const auto &[key, rows] : groups) {
      std::vector<SqlValue> aggregateValues;
      if (aggregate.function != AggregateFunction::Count) {
        aggregateValues.reserve(rows.size());
        for (const auto *row : rows) {
          aggregateValues.push_back((*row)[*plan.aggregateIndex]);
        }
      }
      SqlValue aggregateValue =
          calculateAggregate(aggregate.function, aggregateValues, rows.size());

      if (select.having.has_value()) {
        const int comparison =
            compareSqlValues(aggregateValue, select.having->value);
        bool matches = false;
        switch (select.having->comparison) {
        case ComparisonOperator::Equal:
          matches = comparison == 0;
          break;
        case ComparisonOperator::NotEqual:
          matches = comparison != 0;
          break;
        case ComparisonOperator::GreaterThan:
          matches = comparison > 0;
          break;
        case ComparisonOperator::LessThan:
          matches = comparison < 0;
          break;
        case ComparisonOperator::GreaterThanOrEqual:
          matches = comparison >= 0;
          break;
        case ComparisonOperator::LessThanOrEqual:
          matches = comparison <= 0;
          break;
        default:
          throw std::invalid_argument(
              "HAVING supports only basic comparison operators");
        }
        if (!matches) {
          continue;
        }
      }

      auto outputRow = key;
      outputRow.push_back(std::move(aggregateValue));
      result.rows.push_back(std::move(outputRow));
    }

    if (!plan.groupedOrderIndexes.empty()) {
      std::stable_sort(
          result.rows.begin(), result.rows.end(),
          [&](const auto &left, const auto &right) {
            for (const auto &order : plan.groupedOrderIndexes) {
              const int comparison =
                  compareSqlValues(left[order.index], right[order.index]);
              if (comparison != 0) {
                return order.ascending ? comparison < 0 : comparison > 0;
              }
            }
            return false;
          });
    }

    const std::size_t offset = select.offset > result.rows.size()
                                   ? result.rows.size()
                                   : static_cast<std::size_t>(select.offset);
    if (offset > 0) {
      result.rows.erase(result.rows.begin(), result.rows.begin() + offset);
    }
    if (select.limit.has_value() && *select.limit < result.rows.size()) {
      result.rows.resize(static_cast<std::size_t>(*select.limit));
    }
    return result;
  }

  if (select.aggregate.has_value()) {
    const auto &aggregate = *select.aggregate;
    const std::string functionName =
        aggregate.function == AggregateFunction::Count ? "COUNT"
        : aggregate.function == AggregateFunction::Min ? "MIN"
        : aggregate.function == AggregateFunction::Max ? "MAX"
        : aggregate.function == AggregateFunction::Sum ? "SUM"
                                                       : "AVG";
    const std::string outputName = aggregate.alias.value_or(
        functionName + "(" +
        (aggregate.columnName.has_value() ? *aggregate.columnName : "*") + ")");
    QueryExecutionResult result;
    result.kind = QueryExecutionResultKind::Rows;
    result.message = "Rows selected";
    const SqlType outputType =
        aggregate.function == AggregateFunction::Min ||
                aggregate.function == AggregateFunction::Max
            ? plan.types[*plan.aggregateIndex]
            : SqlType::Int;
    result.columns.push_back({outputName, outputType});

    std::vector<SqlValue> aggregateValues;
    if (aggregate.function != AggregateFunction::Count) {
      aggregateValues.reserve(matchedRows.size());
      for (const auto &row : matchedRows) {
        aggregateValues.push_back(row[*plan.aggregateIndex]);
      }
    }
    result.rows.push_back({calculateAggregate(
        aggregate.function, aggregateValues, matchedRows.size())});
    return result;
  }

  const bool hasIntegerExpression = std::any_of(
      select.integerExpressions.begin(), select.integerExpressions.end(),
      [](const auto &expression) { return expression.has_value(); });
  if (hasIntegerExpression && select.distinct) {
    throw std::invalid_argument(
        "SELECT DISTINCT cannot be combined with integer expressions yet");
  }

  if (select.distinct) {
    std::set<std::vector<SqlValue>> seenProjections;
    matchedRows.erase(
        std::remove_if(
            matchedRows.begin(), matchedRows.end(),
            [&](const auto &row) {
              std::vector<SqlValue> projection;
              projection.reserve(plan.selectedIndexes.size());
              for (const auto index : plan.selectedIndexes) {
                projection.push_back(row[index]);
              }
              return !seenProjections.insert(std::move(projection)).second;
            }),
        matchedRows.end());
  }

  const auto projectRow = [&](const std::vector<SqlValue> &decoded) {
    std::vector<SqlValue> projected;
    projected.reserve(plan.selectedIndexes.size());
    for (std::size_t index = 0; index < plan.selectedIndexes.size(); ++index) {
      if (index < plan.compiledTextFunctions.size() &&
          plan.compiledTextFunctions[index].has_value()) {
        projected.push_back(
            evaluateTextFunction(decoded, *plan.compiledTextFunctions[index]));
      } else if (index < plan.compiledIntegers.size() &&
                 plan.compiledIntegers[index].has_value()) {
        projected.push_back(
            evaluateIntegerExpression(decoded, *plan.compiledIntegers[index]));
      } else if (index < plan.compiledCases.size() &&
                 plan.compiledCases[index].has_value()) {
        const auto &caseExpression = *select.caseExpressions[index];
        bool matched = false;
        for (std::size_t branchIndex = 0;
             branchIndex < plan.compiledCases[index]->size(); ++branchIndex) {
          if (matchesWhereExpression(
                  decoded, (*plan.compiledCases[index])[branchIndex])) {
            projected.push_back(caseExpression.branches[branchIndex].result);
            matched = true;
            break;
          }
        }
        if (!matched) {
          projected.push_back(caseExpression.elseResult);
        }
      } else {
        projected.push_back(decoded[plan.selectedIndexes[index]]);
      }
    }
    return projected;
  };

  const bool ordersByProjection =
      std::any_of(plan.orderIndexes.begin(), plan.orderIndexes.end(),
                  [](const auto &order) { return order.projected; });
  std::vector<std::vector<SqlValue>> projectedRows;
  if (ordersByProjection) {
    projectedRows.reserve(matchedRows.size());
    for (const auto &row : matchedRows) {
      projectedRows.push_back(projectRow(row));
    }
  }

  if (!plan.orderIndexes.empty()) {
    std::vector<std::size_t> rowOrder(matchedRows.size());
    for (std::size_t index = 0; index < rowOrder.size(); ++index) {
      rowOrder[index] = index;
    }
    std::stable_sort(
        rowOrder.begin(), rowOrder.end(),
        [&](std::size_t leftIndex, std::size_t rightIndex) {
          for (const auto &order : plan.orderIndexes) {
            const auto &left = order.projected
                                   ? projectedRows[leftIndex][order.index]
                                   : matchedRows[leftIndex][order.index];
            const auto &right = order.projected
                                    ? projectedRows[rightIndex][order.index]
                                    : matchedRows[rightIndex][order.index];
            const int comparison = compareSqlValues(left, right);
            if (comparison != 0) {
              return order.ascending ? comparison < 0 : comparison > 0;
            }
          }
          return false;
        });

    std::vector<std::vector<SqlValue>> sortedRows;
    std::vector<std::vector<SqlValue>> sortedProjections;
    sortedRows.reserve(matchedRows.size());
    if (ordersByProjection) {
      sortedProjections.reserve(projectedRows.size());
    }
    for (const auto index : rowOrder) {
      sortedRows.push_back(std::move(matchedRows[index]));
      if (ordersByProjection) {
        sortedProjections.push_back(std::move(projectedRows[index]));
      }
    }
    matchedRows = std::move(sortedRows);
    if (ordersByProjection) {
      projectedRows = std::move(sortedProjections);
    }
  }

  const std::size_t offset = select.offset > matchedRows.size()
                                 ? matchedRows.size()
                                 : static_cast<std::size_t>(select.offset);
  if (offset > 0) {
    matchedRows.erase(matchedRows.begin(), matchedRows.begin() + offset);
    if (ordersByProjection) {
      projectedRows.erase(projectedRows.begin(),
                          projectedRows.begin() + offset);
    }
  }
  if (select.limit.has_value() && *select.limit < matchedRows.size()) {
    const auto limit = static_cast<std::size_t>(*select.limit);
    matchedRows.resize(limit);
    if (ordersByProjection) {
      projectedRows.resize(limit);
    }
  }
  if (!ordersByProjection) {
    projectedRows.reserve(matchedRows.size());
    for (const auto &row : matchedRows) {
      projectedRows.push_back(projectRow(row));
    }
  }

  QueryExecutionResult result;
  result.kind = QueryExecutionResultKind::Rows;
  result.message = "Rows selected";
  result.columns = plan.selectedColumns;
  result.rows = std::move(projectedRows);
  return result;
}

} // namespace QueryExecutionService::domain
