#pragma once

#include "SqlSelectPlanner.h"

#include <vector>

namespace QueryExecutionService::domain {

QueryExecutionService::QueryExecutionResult processSelectRows(
    const unidb::sql::SelectStatement &select, const SelectPlan &plan,
    std::vector<std::vector<unidb::sql::SqlValue>> matchedRows);

} // namespace QueryExecutionService::domain
