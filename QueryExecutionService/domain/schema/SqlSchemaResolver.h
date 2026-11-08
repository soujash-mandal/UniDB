#pragma once

#include "../contract/sqlStatement.h"
#include "../../type/type.h"

#include <string>
#include <vector>

namespace QueryExecutionService::domain {

std::vector<unidb::sql::SqlType>
schemaTypes(const std::vector<unidb::sql::ColumnDefinition> &schema);

ResolvedJoinColumn
resolveJoinColumn(const unidb::sql::ColumnReference &reference,
                  const std::string &leftTableName,
                  const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
                  const std::string &rightTableName,
                  const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

ResolvedJoinColumn
resolveSelectedJoinColumn(
    const std::string &name, const std::string &leftTableName,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::string &rightTableName,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

ResolvedJoinColumn
resolveWhereJoinColumn(
    const std::string &name, const std::string &leftTableName,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::string &rightTableName,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

std::string normalizeSingleTableColumnName(const std::string &name,
                                           const std::string &tableQualifier);

} // namespace QueryExecutionService::domain
