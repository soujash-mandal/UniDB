#pragma once

#include "../contract/sqlStatement.h"

#include <string>
#include <vector>

namespace QueryExecutionService::domain {

unidb::sql::WhereExpression
normalizeSingleTableWhereColumns(const unidb::sql::WhereExpression &expression,
                                 const std::string &tableName);

unidb::sql::WhereExpression qualifyJoinWhereColumns(
    const unidb::sql::WhereExpression &expression,
    const std::string &leftTableName,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::string &rightTableName,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

} // namespace QueryExecutionService::domain
