#pragma once

#include "../contract/sqlStatement.h"

#include <vector>

namespace QueryExecutionService::domain {

using QueryExecutionResult = QueryExecutionService::QueryExecutionResult;

void validateCreateTable(const unidb::sql::CreateTableStatement &statement);

std::vector<std::vector<unidb::sql::SqlValue>>
prepareInsertRows(const unidb::sql::InsertStatement &statement,
                  const std::vector<unidb::sql::ColumnDefinition> &schema);

QueryExecutionResult
combineUnionResults(const std::vector<QueryExecutionResult> &branches,
                    bool removeDuplicates);

} // namespace QueryExecutionService::domain
