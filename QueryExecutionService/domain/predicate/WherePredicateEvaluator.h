#pragma once

#include "../../contract/sqlStatement.h"

#include <cstddef>
#include <vector>

namespace QueryExecutionService::domain {

struct CompiledWhereExpression {
  unidb::sql::WhereExpression::Kind kind{
      unidb::sql::WhereExpression::Kind::Predicate};
  std::size_t columnIndex{0};
  unidb::sql::WhereEquals predicate;
  std::vector<CompiledWhereExpression> children;
};

int compareSqlValues(const unidb::sql::SqlValue &left,
                     const unidb::sql::SqlValue &right);

CompiledWhereExpression compileWhereExpression(
    const unidb::sql::WhereExpression &expression,
    const std::vector<unidb::sql::ColumnDefinition> &schema,
    const std::vector<unidb::sql::SqlType> &types);

bool matchesWhereExpression(
    const std::vector<unidb::sql::SqlValue> &row,
    const CompiledWhereExpression &expression);

} // namespace QueryExecutionService::domain
