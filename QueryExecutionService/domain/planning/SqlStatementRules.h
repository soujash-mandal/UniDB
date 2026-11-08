#pragma once

#include "../contract/sqlStatement.h"

#include <vector>

namespace QueryExecutionService::domain {

void validateCreateTable(const unidb::sql::CreateTableStatement &statement);

std::vector<std::vector<unidb::sql::SqlValue>>
prepareInsertRows(const unidb::sql::InsertStatement &statement,
                  const std::vector<unidb::sql::ColumnDefinition> &schema);

QueryExecutionService::QueryExecutionResult
combineUnionResults(const std::vector<QueryExecutionService::QueryExecutionResult> &branches,
                    bool removeDuplicates);

} // namespace QueryExecutionService::domain
