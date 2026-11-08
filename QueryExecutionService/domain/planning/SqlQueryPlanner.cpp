#include "SqlQueryPlanner.h"

#include "../expression/SqlExpressionCompiler.h"
#include "../aggregate/SqlAggregateCalculator.h"
#include "../predicate/WhereExpressionNormalizer.h"
#include "../predicate/WherePredicateEvaluator.h"
#include "../schema/SqlSchemaResolver.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace QueryExecutionService::domain {

using namespace unidb::sql;

void validateCreateTable(const CreateTableStatement &statement) {
  if (statement.columns.empty()) {
    throw std::invalid_argument("CREATE TABLE requires at least one column");
  }
  std::unordered_set<std::string> names;
  for (const auto &column : statement.columns) {
    if (column.name.empty() || !names.insert(column.name).second) {
      throw std::invalid_argument("Column names must be non-empty and unique");
    }
    if (column.type != SqlType::Int && column.type != SqlType::Text) {
      throw std::invalid_argument("Column type is outside the V1 SQL subset");
    }
  }
}

std::vector<std::vector<SqlValue>>
prepareInsertRows(const InsertStatement &statement,
                  const std::vector<ColumnDefinition> &schema) {
  const auto types = schemaTypes(schema);
  if (statement.rows.empty()) {
    throw std::invalid_argument("INSERT requires at least one VALUES row");
  }
  std::vector<std::vector<SqlValue>> orderedRows;
  orderedRows.reserve(statement.rows.size());
  for (const auto &values : statement.rows) {
    if (statement.columnNames.size() != values.size()) {
      throw std::invalid_argument("INSERT column and value counts must match");
    }
    std::vector<SqlValue> orderedValues(schema.size());
    std::vector<bool> assigned(schema.size(), false);
    for (std::size_t inputIndex = 0; inputIndex < statement.columnNames.size();
         ++inputIndex) {
      const auto found = std::find_if(
          schema.begin(), schema.end(), [&](const auto &column) {
            return column.name == statement.columnNames[inputIndex];
          });
      if (found == schema.end()) {
        throw std::invalid_argument("Unknown INSERT column: " +
                                    statement.columnNames[inputIndex]);
      }
      const auto schemaIndex =
          static_cast<std::size_t>(std::distance(schema.begin(), found));
      if (assigned[schemaIndex]) {
        throw std::invalid_argument("Duplicate INSERT column: " +
                                    statement.columnNames[inputIndex]);
      }
      const bool valueIsInt =
          std::holds_alternative<std::int64_t>(values[inputIndex]);
      if ((types[schemaIndex] == SqlType::Int) != valueIsInt) {
        throw std::invalid_argument(
            "INSERT value type does not match column: " + found->name);
      }
      orderedValues[schemaIndex] = values[inputIndex];
      assigned[schemaIndex] = true;
    }
    if (std::find(assigned.begin(), assigned.end(), false) != assigned.end()) {
      throw std::invalid_argument(
          "V1 INSERT requires a value for every table column");
    }
    orderedRows.push_back(std::move(orderedValues));
  }
  return orderedRows;
}

QueryExecutionResult combineUnionResults(
    const std::vector<QueryExecutionResult> &branches, bool removeDuplicates) {
  if (branches.size() < 2) {
    throw std::invalid_argument(removeDuplicates
                                    ? "UNION requires at least two SELECT branches"
                                    : "UNION ALL requires at least two SELECT branches");
  }
  QueryExecutionResult combined;
  combined.kind = QueryExecutionResultKind::Rows;
  combined.message = "Rows selected";
  combined.columns = branches.front().columns;
  std::set<std::vector<SqlValue>> seenRows;
  for (const auto &branch : branches) {
    if (branch.kind != QueryExecutionResultKind::Rows) {
      throw std::invalid_argument(removeDuplicates
                                      ? "UNION branches must return rows"
                                      : "UNION ALL branches must return rows");
    }
    if (branch.columns.size() != combined.columns.size()) {
      throw std::invalid_argument(removeDuplicates
                                      ? "UNION branches must return the same number of columns"
                                      : "UNION ALL branches must return the same number of columns");
    }
    for (std::size_t index = 0; index < combined.columns.size(); ++index) {
      if (branch.columns[index].type != combined.columns[index].type) {
        throw std::invalid_argument(removeDuplicates
                                        ? "UNION columns must have matching types at each position"
                                        : "UNION ALL columns must have matching types at each position");
      }
    }
    if (removeDuplicates) {
      for (const auto &row : branch.rows) {
        if (seenRows.insert(row).second) {
          combined.rows.push_back(row);
        }
      }
    } else {
      combined.rows.insert(combined.rows.end(), branch.rows.begin(),
                           branch.rows.end());
    }
  }
  return combined;
}

SelectPlan buildSelectPlan(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &schema) {
  SelectPlan plan;
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
          const auto compiled = compileTextFunction(
              expression, schema, types, tableQualifier);
          plan.selectedIndexes.push_back(schema.size());
          const auto alias = itemIndex < select.columnAliases.size()
                                 ? select.columnAliases[itemIndex]
                                 : std::optional<std::string>{};
          const auto functionName =
              textFunctionName(expression.function);
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
          const auto compiled = compileIntegerExpression(
              *select.integerExpressions[itemIndex], schema, types,
              tableQualifier);
          plan.selectedIndexes.push_back(schema.size());
          const auto alias = itemIndex < select.columnAliases.size()
                                 ? select.columnAliases[itemIndex]
                                 : std::optional<std::string>{};
          plan.selectedColumns.push_back(
              {alias.value_or("EXPR" + std::to_string(itemIndex + 1)),
               SqlType::Int});
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
            branches.push_back(
                compileWhereExpression(normalized, schema, types));
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
        throw std::invalid_argument(
            "SELECT alias count does not match columns");
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
        const auto alias =
            std::find_if(plan.selectedColumns.begin(), plan.selectedColumns.end(),
                         [&](const auto &column) {
                           return column.name == order.columnName;
                         });
        if (alias != plan.selectedColumns.end()) {
          const auto projectionIndex = static_cast<std::size_t>(
              std::distance(plan.selectedColumns.begin(), alias));
          plan.orderIndexes.push_back({projectionIndex, order.ascending, true});
          continue;
        }

        const auto normalizedOrderColumn =
            normalizeSingleTableColumnName(order.columnName,
                                                   tableQualifier);
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
        plan.orderIndexes.push_back({orderIndex, order.ascending, false});
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
      if (select.distinct) {
        throw std::invalid_argument(
            "Grouped aggregate SELECT cannot be combined with DISTINCT");
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
          throw std::invalid_argument("Unknown GROUP BY column: " +
                                      groupColumn);
        }
        plan.groupIndexes.push_back(
            static_cast<std::size_t>(std::distance(schema.begin(), found)));
      }
    }
    if (select.aggregate.has_value()) {
      const auto &aggregate = *select.aggregate;
      if (select.distinct ||
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
          (aggregate.columnName.has_value() ? *aggregate.columnName : "*") +
          ")");
      plan.groupedOrderIndexes.reserve(select.orderBy.size());
      for (const auto &order : select.orderBy) {
        const auto projected =
            std::find_if(plan.selectedColumns.begin(), plan.selectedColumns.end(),
                         [&](const auto &column) {
                           return column.name == order.columnName;
                         });
        if (projected != plan.selectedColumns.end()) {
          plan.groupedOrderIndexes.push_back(
              {static_cast<std::size_t>(
                   std::distance(plan.selectedColumns.begin(), projected)),
               order.ascending});
          continue;
        }
        if (order.columnName == aggregateName) {
          plan.groupedOrderIndexes.push_back(
              {plan.selectedColumns.size(), order.ascending});
          continue;
        }

        const auto normalizedOrderColumn =
            normalizeSingleTableColumnName(order.columnName,
                                                   tableQualifier);
        const auto groupIndex = std::find_if(
            plan.groupIndexes.begin(), plan.groupIndexes.end(), [&](const auto index) {
              return schema[index].name == normalizedOrderColumn;
            });
        if (groupIndex == plan.groupIndexes.end()) {
          throw std::invalid_argument("Unknown GROUP BY ORDER BY column: " +
                                      order.columnName);
        }
        plan.groupedOrderIndexes.push_back({static_cast<std::size_t>(std::distance(
                                           plan.groupIndexes.begin(), groupIndex)),
                                       order.ascending});
      }
    }
  return plan;
}

unidb::sql::QueryExecutionResult processSelectRows(
    const unidb::sql::SelectStatement &select, const SelectPlan &plan,
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
          (aggregate.columnName.has_value() ? *aggregate.columnName : "*") +
          ")");
      if (std::any_of(result.columns.begin(), result.columns.end(),
                      [&](const auto &column) {
                        return column.name == aggregateName;
                      })) {
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
        SqlValue aggregateValue = calculateAggregate(
            aggregate.function, aggregateValues, rows.size());

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
        std::stable_sort(result.rows.begin(), result.rows.end(),
                         [&](const auto &left, const auto &right) {
                           for (const auto &order : plan.groupedOrderIndexes) {
                             const int comparison = compareSqlValues(
                                 left[order.index], right[order.index]);
                             if (comparison != 0) {
                               return order.ascending ? comparison < 0
                                                      : comparison > 0;
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
          (aggregate.columnName.has_value() ? *aggregate.columnName : "*") +
          ")");
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
          projected.push_back(evaluateTextFunction(
              decoded, *plan.compiledTextFunctions[index]));
        } else if (index < plan.compiledIntegers.size() &&
                   plan.compiledIntegers[index].has_value()) {
          projected.push_back(evaluateIntegerExpression(
              decoded, *plan.compiledIntegers[index]));
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
