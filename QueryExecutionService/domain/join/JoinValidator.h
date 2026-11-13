#pragma once

#include "../contract/sqlStatement.h"

#include <vector>

namespace QueryExecutionService::domain {

void validateInnerJoinColumns(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

void validateLeftJoinColumns(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

void validateFullJoinColumns(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

void validateRightJoinColumns(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema);

} // namespace QueryExecutionService::domain
