#pragma once

#include "../contract/sqlStatement.h"

#include <cstddef>
#include <vector>

namespace QueryExecutionService::domain {

unidb::sql::SqlValue
calculateAggregate(unidb::sql::AggregateFunction function,
                   const std::vector<unidb::sql::SqlValue> &values,
                   std::size_t inputRowCount, bool countAllRows = false);

} // namespace QueryExecutionService::domain
