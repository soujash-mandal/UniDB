#pragma once

#include "../contract/sqlStatement.h"

#include <cstddef>
#include <string>
#include <vector>

namespace QueryExecutionService::domain {

struct JoinProjectionPlan {
  std::vector<std::size_t> indexes;
  std::vector<unidb::sql::ColumnDefinition> columns;
};

JoinProjectionPlan buildJoinProjectionPlan(
    const unidb::sql::SelectStatement &select,
    const std::vector<std::vector<unidb::sql::ColumnDefinition>> &schemas,
    const std::vector<std::string> &qualifiers);

} // namespace QueryExecutionService::domain
