#include "JoinValidator.h"

#include "../schema/SqlSchemaResolver.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace QueryExecutionService::domain {
using unidb::sql::ColumnDefinition;
using unidb::sql::SelectStatement;

  if (const auto *select = std::get_if<SelectStatement>(&statement)) {
    if (select->crossJoin.has_value()) {
      if (!select->innerJoins.empty()) {
        throw std::invalid_argument(
            "Mixing CROSS JOIN and INNER JOIN is not supported");
      }
      const auto leftSchema =
          rdbServicePort.getTableDefinition(select->tableName);
      const auto rightSchema =
          rdbServicePort.getTableDefinition(select->crossJoin->tableName);
      domain::schemaTypes(leftSchema);
      domain::schemaTypes(rightSchema);
      return executeCrossJoin(rdbServicePort, *select, leftSchema, rightSchema);
    }
    if (!select->innerJoins.empty()) {
      std::vector<std::vector<ColumnDefinition>> schemas;
      schemas.push_back(rdbServicePort.getTableDefinition(select->tableName));
      for (const auto &join : select->innerJoins)
        schemas.push_back(rdbServicePort.getTableDefinition(join.tableName));
      for (const auto &schema : schemas)
        domain::schemaTypes(schema);
      if (select->innerJoins.size() == 1) {
        domain::validateInnerJoinColumns(*select, schemas[0], schemas[1]);
        return executeInnerJoin(rdbServicePort, *select, schemas[0],
                                schemas[1]);
      }
      return executeMultipleInnerJoins(rdbServicePort, *select, schemas);
    }
    const auto schema = rdbServicePort.getTableDefinition(select->tableName);
    const auto types = domain::schemaTypes(schema);

    const auto *where = std::get_if<WhereExpression>(&select->where);
    std::optional<domain::CompiledWhereExpression> compiledWhere;
    if (where != nullptr) {
      const std::string tableQualifier =
          select->tableAlias.value_or(select->tableName);
      const auto normalized =
          domain::normalizeSingleTableWhereColumns(*where, tableQualifier);
      compiledWhere = domain::compileWhereExpression(normalized, schema, types);
    }

} // namespace QueryExecutionService::domain
