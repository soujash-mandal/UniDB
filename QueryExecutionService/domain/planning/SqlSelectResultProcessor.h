#pragma once

#include "SqlSelectPlanner.h"

#include <optional>
#include <vector>

namespace QueryExecutionService::domain {

std::vector<std::vector<unidb::sql::SqlValue>> decodeAndFilterRows(
    const std::vector<std::vector<char>> &encodedRows,
    const std::vector<unidb::sql::SqlType> &types,
    const std::optional<CompiledWhereExpression> &compiledWhere);

QueryExecutionService::QueryExecutionResult
processSelectRows(const unidb::sql::SelectStatement &select,
                  const SelectPlan &plan,
                  std::vector<std::vector<unidb::sql::SqlValue>> matchedRows);

} // namespace QueryExecutionService::domain
