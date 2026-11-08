#pragma once

#include "../contract/sqlStatement.h"
#include "../expression/ScalarExpressionEvaluator.h"
#include "../predicate/WherePredicateEvaluator.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace QueryExecutionService::domain {

struct SelectPlan {
  struct OrderIndex {
    std::size_t index;
    bool ascending;
    bool projected;
  };
  struct GroupOrderIndex {
    std::size_t index;
    bool ascending;
  };

  std::vector<unidb::sql::SqlType> types;
  std::optional<CompiledWhereExpression> compiledWhere;
  std::vector<std::size_t> selectedIndexes;
  std::vector<unidb::sql::ColumnDefinition> selectedColumns;
  std::vector<std::optional<std::vector<CompiledWhereExpression>>>
      compiledCases;
  std::vector<std::optional<CompiledIntegerExpression>> compiledIntegers;
  std::vector<std::optional<CompiledTextFunction>> compiledTextFunctions;
  std::vector<OrderIndex> orderIndexes;
  std::optional<std::size_t> aggregateIndex;
  std::vector<std::size_t> groupIndexes;
  std::vector<GroupOrderIndex> groupedOrderIndexes;
};

SelectPlan buildSelectPlan(const unidb::sql::SelectStatement &select,
                           const std::vector<unidb::sql::ColumnDefinition> &schema);

} // namespace QueryExecutionService::domain
