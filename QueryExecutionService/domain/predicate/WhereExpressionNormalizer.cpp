#include "WhereExpressionNormalizer.h"

#include "../schema/SqlSchemaResolver.h"

namespace QueryExecutionService::domain {
using unidb::sql::WhereExpression;

WhereExpression
normalizeSingleTableWhereColumns(const WhereExpression &expression,
                                 const std::string &tableName) {
  WhereExpression normalized = expression;
  if (normalized.kind == WhereExpression::Kind::Predicate) {
    const auto separator = normalized.predicate.columnName.find('.');
    if (separator != std::string::npos) {
      const std::string qualifier =
          normalized.predicate.columnName.substr(0, separator);
      if (qualifier != tableName ||
          normalized.predicate.columnName.find('.', separator + 1) !=
              std::string::npos) {
        throw std::invalid_argument("Unknown WHERE table reference: " +
                                    qualifier);
      }
      normalized.predicate.columnName =
          normalized.predicate.columnName.substr(separator + 1);
    }
    return normalized;
  }
  for (auto &child : normalized.children) {
    child = normalizeSingleTableWhereColumns(child, tableName);
  }
  return normalized;
}

WhereExpression
qualifyJoinWhereColumns(const WhereExpression &expression,
                        const std::string &leftTableName,
                        const std::vector<ColumnDefinition> &leftSchema,
                        const std::string &rightTableName,
                        const std::vector<ColumnDefinition> &rightSchema) {
  WhereExpression qualified = expression;
  if (qualified.kind == WhereExpression::Kind::Predicate) {
    const auto resolved =
        resolveWhereJoinColumn(qualified.predicate.columnName, leftTableName,
                               leftSchema, rightTableName, rightSchema);
    qualified.predicate.columnName =
        resolved.tableName + "." + resolved.column->name;
    return qualified;
  }
  for (auto &child : qualified.children) {
    child = qualifyJoinWhereColumns(child, leftTableName, leftSchema,
                                    rightTableName, rightSchema);
  }
  return qualified;
}

} // namespace QueryExecutionService::domain
