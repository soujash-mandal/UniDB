#pragma once

#include "../../type/type.h"
#include "../contract/sqlStatement.h"

#include <vector>

namespace QueryExecutionService::domain {

QueryExecutionResult executeInnerJoin(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema,
    const std::vector<std::vector<unidb::sql::SqlValue>> &leftRows,
    const std::vector<std::vector<unidb::sql::SqlValue>> &rightRows);

QueryExecutionResult executeLeftJoin(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema,
    const std::vector<std::vector<unidb::sql::SqlValue>> &leftRows,
    const std::vector<std::vector<unidb::sql::SqlValue>> &rightRows);

QueryExecutionResult executeRightJoin(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema,
    const std::vector<std::vector<unidb::sql::SqlValue>> &leftRows,
    const std::vector<std::vector<unidb::sql::SqlValue>> &rightRows);

QueryExecutionResult executeFullJoin(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema,
    const std::vector<std::vector<unidb::sql::SqlValue>> &leftRows,
    const std::vector<std::vector<unidb::sql::SqlValue>> &rightRows);

QueryExecutionResult executeCrossJoin(
    const unidb::sql::SelectStatement &select,
    const std::vector<unidb::sql::ColumnDefinition> &leftSchema,
    const std::vector<unidb::sql::ColumnDefinition> &rightSchema,
    const std::vector<std::vector<unidb::sql::SqlValue>> &leftRows,
    const std::vector<std::vector<unidb::sql::SqlValue>> &rightRows);

QueryExecutionResult executeMultipleInnerJoins(
    const unidb::sql::SelectStatement &select,
    const std::vector<std::vector<unidb::sql::ColumnDefinition>> &schemas,
    const std::vector<std::vector<std::vector<unidb::sql::SqlValue>>>
        &rowsByTable);

} // namespace QueryExecutionService::domain
