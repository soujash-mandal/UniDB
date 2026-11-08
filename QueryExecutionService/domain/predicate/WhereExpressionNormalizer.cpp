#include "WhereExpressionNormalizer.h"

#include "../schema/SqlSchemaResolver.h"

namespace QueryExecutionService::domain {
using unidb::sql::WhereExpression;

    const auto *where = std::get_if<WhereExpression>(&select->where);
    std::optional<domain::CompiledWhereExpression> compiledWhere;
    if (where != nullptr) {
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      const auto normalized =
          domain::normalizeSingleTableWhereColumns(*where, tableQualifier);
      compiledWhere = domain::compileWhereExpression(normalized, schema, types);
    }

    std::vector<std::size_t> selectedIndexes;
    std::vector<ColumnDefinition> selectedColumns;
    std::vector<std::optional<std::vector<domain::CompiledWhereExpression>>>
        compiledCases;
    std::vector<std::optional<domain::CompiledIntegerExpression>>
        compiledIntegers;
    std::vector<std::optional<domain::CompiledTextFunction>>
        compiledTextFunctions;
    if (select->aggregate.has_value() && select->groupBy.empty()) {
      // Ungrouped aggregate queries produce a single computed output column.
    }

  std::optional<domain::CompiledWhereExpression> compiledWhere;
  if (const auto *where = std::get_if<WhereExpression>(&select.where)) {
    const auto qualified = domain::qualifyJoinWhereColumns(
        *where, leftQualifier, leftSchema, rightQualifier, rightSchema);
    compiledWhere =
        domain::compileWhereExpression(qualified, joinedSchema, joinedTypes);
  }

  std::vector<std::size_t> selectedIndexes;
  std::vector<ColumnDefinition> selectedColumns;
  if (select.selectAll) {
    selectedIndexes.reserve(joinedSchema.size());
    selectedColumns.reserve(joinedSchema.size());
    for (std::size_t index = 0; index < joinedSchema.size(); ++index) {
      selectedIndexes.push_back(index);
      const bool isLeft = index < leftSchema.size();
      const auto &sourceColumn =
          isLeft ? leftSchema[index] : rightSchema[index - leftSchema.size()];
      const auto &otherSchema = isLeft ? rightSchema : leftSchema;
      const bool duplicated = std::any_of(
          otherSchema.begin(), otherSchema.end(),
          [&](const auto &other) { return other.name == sourceColumn.name; });
      const std::string &tableName = isLeft ? leftQualifier : rightQualifier;
      selectedColumns.push_back(
          {duplicated ? tableName + "." + sourceColumn.name : sourceColumn.name,
           sourceColumn.type});
    }
  }

} // namespace QueryExecutionService::domain
