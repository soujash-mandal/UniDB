#include "SqlQueryRules.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace QueryExecutionService::domain {

std::vector<SqlType> schemaTypes(const std::vector<ColumnDefinition> &schema) {
  std::vector<SqlType> types;
  types.reserve(schema.size());
  for (const auto &column : schema) {
    if (column.type != SqlType::Int && column.type != SqlType::Text) {
      throw std::invalid_argument("Column type is outside the V1 SQL subset");
    }
    types.push_back(column.type);
  }
  return types;
}

ResolvedJoinColumn
resolveJoinColumn(const unidb::sql::ColumnReference &reference,
                  const std::string &leftTableName,
                  const std::vector<ColumnDefinition> &leftSchema,
                  const std::string &rightTableName,
                  const std::vector<ColumnDefinition> &rightSchema) {
  if (!reference.tableName.has_value()) {
    throw std::invalid_argument(
        "JOIN ON requires table-qualified column references");
  }

  const std::vector<ColumnDefinition> *schema = nullptr;
  if (*reference.tableName == leftTableName) {
    schema = &leftSchema;
  } else if (*reference.tableName == rightTableName) {
    schema = &rightSchema;
  } else {
    throw std::invalid_argument("Unknown JOIN table reference: " +
                                *reference.tableName);
  }

  const auto found =
      std::find_if(schema->begin(), schema->end(), [&](const auto &column) {
        return column.name == reference.columnName;
      });
  if (found == schema->end()) {
    throw std::invalid_argument("Unknown JOIN column: " + *reference.tableName +
                                "." + reference.columnName);
  }

  return {&*found, *reference.tableName,
          static_cast<std::size_t>(std::distance(schema->begin(), found))};
}

ResolvedJoinColumn
resolveSelectedJoinColumn(const std::string &name,
                          const std::string &leftTableName,
                          const std::vector<ColumnDefinition> &leftSchema,
                          const std::string &rightTableName,
                          const std::vector<ColumnDefinition> &rightSchema) {
  const auto separator = name.find('.');
  if (separator != std::string::npos) {
    if (name.find('.', separator + 1) != std::string::npos) {
      throw std::invalid_argument("Invalid qualified SELECT column: " + name);
    }
    const std::string tableName = name.substr(0, separator);
    const std::string columnName = name.substr(separator + 1);
    if (tableName != leftTableName && tableName != rightTableName) {
      throw std::invalid_argument("Unknown SELECT table: " + tableName);
    }
    const auto &schema = tableName == leftTableName ? leftSchema : rightSchema;
    const auto found =
        std::find_if(schema.begin(), schema.end(), [&](const auto &column) {
          return column.name == columnName;
        });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown SELECT column: " + name);
    }
    return {&*found, tableName,
            static_cast<std::size_t>(std::distance(schema.begin(), found))};
  }

  const auto leftFound =
      std::find_if(leftSchema.begin(), leftSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const auto rightFound =
      std::find_if(rightSchema.begin(), rightSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const bool inLeft = leftFound != leftSchema.end();
  const bool inRight = rightFound != rightSchema.end();

  if (!inLeft && !inRight) {
    throw std::invalid_argument("Unknown SELECT column: " + name);
  }
  if (inLeft && inRight) {
    throw std::invalid_argument("Ambiguous SELECT column: " + name);
  }
  if (inLeft) {
    return {
        &*leftFound, leftTableName,
        static_cast<std::size_t>(std::distance(leftSchema.begin(), leftFound))};
  }
  return {
      &*rightFound, rightTableName,
      static_cast<std::size_t>(std::distance(rightSchema.begin(), rightFound))};
}

void validateInnerJoinColumns(
    const SelectStatement &select,
    const std::vector<ColumnDefinition> &leftSchema,
    const std::vector<ColumnDefinition> &rightSchema) {
  const auto &join = select.innerJoins.front();
  const std::string leftQualifier =
      select.tableAlias.value_or(select.tableName);
  const std::string rightQualifier = join.tableAlias.value_or(join.tableName);
  if (leftQualifier == rightQualifier) {
    throw std::invalid_argument("JOIN table aliases must be distinct");
  }
  if (select.tableName == join.tableName &&
      (!select.tableAlias.has_value() || !join.tableAlias.has_value())) {
    throw std::invalid_argument(
        "Joining a table to itself requires distinct table aliases");
  }

  const auto leftKey = resolveJoinColumn(join.left, leftQualifier, leftSchema,
                                         rightQualifier, rightSchema);
  const auto rightKey = resolveJoinColumn(join.right, leftQualifier, leftSchema,
                                          rightQualifier, rightSchema);
  if (leftKey.tableName == rightKey.tableName) {
    throw std::invalid_argument(
        "INNER JOIN ON must compare columns from both tables");
  }
  if (leftKey.column->type != rightKey.column->type) {
    throw std::invalid_argument("INNER JOIN columns must have matching types");
  }

  if (select.aggregate.has_value() || !select.groupBy.empty() ||
      select.having.has_value() || select.distinct || !select.orderBy.empty() ||
      select.limit.has_value() || select.offset != 0) {
    throw std::invalid_argument(
        "JOIN currently supports only a simple SELECT projection");
  }

  std::vector<std::string> outputNames;
  if (select.selectAll) {
    outputNames.reserve(leftSchema.size() + rightSchema.size());
    for (const auto &column : leftSchema) {
      const bool duplicated = std::any_of(
          rightSchema.begin(), rightSchema.end(),
          [&](const auto &right) { return right.name == column.name; });
      outputNames.push_back(duplicated ? leftQualifier + "." + column.name
                                       : column.name);
    }
    for (const auto &column : rightSchema) {
      const bool duplicated = std::any_of(
          leftSchema.begin(), leftSchema.end(),
          [&](const auto &left) { return left.name == column.name; });
      outputNames.push_back(duplicated ? rightQualifier + "." + column.name
                                       : column.name);
    }
  } else {
    if (select.columnNames.empty()) {
      throw std::invalid_argument("SELECT requires at least one column");
    }
    if (!select.columnAliases.empty() &&
        select.columnAliases.size() != select.columnNames.size()) {
      throw std::invalid_argument("SELECT alias count does not match columns");
    }
    outputNames.reserve(select.columnNames.size());
    for (std::size_t index = 0; index < select.columnNames.size(); ++index) {
      const auto resolved =
          resolveSelectedJoinColumn(select.columnNames[index], leftQualifier,
                                    leftSchema, rightQualifier, rightSchema);
      const bool duplicated =
          std::any_of(leftSchema.begin(), leftSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      }) &&
          std::any_of(rightSchema.begin(), rightSchema.end(),
                      [&](const auto &column) {
                        return column.name == resolved.column->name;
                      });
      std::string outputName =
          duplicated ? resolved.tableName + "." + resolved.column->name
                     : resolved.column->name;
      if (index < select.columnAliases.size() &&
          select.columnAliases[index].has_value()) {
        outputName = *select.columnAliases[index];
      }
      outputNames.push_back(std::move(outputName));
    }
  }

  std::unordered_set<std::string> uniqueOutputNames;
  for (const auto &name : outputNames) {
    if (!uniqueOutputNames.insert(name).second) {
      throw std::invalid_argument("Duplicate SELECT output column name: " +
                                  name);
    }
  }
}

ResolvedJoinColumn
resolveWhereJoinColumn(const std::string &name,
                       const std::string &leftTableName,
                       const std::vector<ColumnDefinition> &leftSchema,
                       const std::string &rightTableName,
                       const std::vector<ColumnDefinition> &rightSchema) {
  const auto separator = name.find('.');
  if (separator != std::string::npos) {
    return resolveSelectedJoinColumn(name, leftTableName, leftSchema,
                                     rightTableName, rightSchema);
  }
  const auto left =
      std::find_if(leftSchema.begin(), leftSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const auto right =
      std::find_if(rightSchema.begin(), rightSchema.end(),
                   [&](const auto &column) { return column.name == name; });
  const bool inLeft = left != leftSchema.end();
  const bool inRight = right != rightSchema.end();
  if (inLeft && inRight) {
    throw std::invalid_argument("Ambiguous WHERE column: " + name);
  }
  if (!inLeft && !inRight) {
    throw std::invalid_argument("Unknown WHERE column: " + name);
  }
  if (inLeft) {
    return {&*left, leftTableName,
            static_cast<std::size_t>(std::distance(leftSchema.begin(), left))};
  }
  return {&*right, rightTableName,
          static_cast<std::size_t>(std::distance(rightSchema.begin(), right))};
}

std::string normalizeSingleTableColumnName(const std::string &name,
                                           const std::string &tableQualifier) {
  const auto separator = name.find('.');
  if (separator == std::string::npos) {
    return name;
  }
  if (name.find('.', separator + 1) != std::string::npos ||
      name.substr(0, separator) != tableQualifier) {
    throw std::invalid_argument("Unknown SELECT table reference: " +
                                name.substr(0, separator));
  }
  return name.substr(separator + 1);
}

CompiledTextFunction
compileTextFunction(const TextFunctionExpression &expression,
                    const std::vector<ColumnDefinition> &schema,
                    const std::vector<SqlType> &types,
                    const std::string &tableQualifier) {
  const auto name =
      normalizeSingleTableColumnName(expression.columnName, tableQualifier);
  const auto found =
      std::find_if(schema.begin(), schema.end(),
                   [&](const auto &column) { return column.name == name; });
  if (found == schema.end()) {
    throw std::invalid_argument("Unknown SELECT column: " +
                                expression.columnName);
  }
  const auto index =
      static_cast<std::size_t>(std::distance(schema.begin(), found));
  if (types[index] != SqlType::Text) {
    throw std::invalid_argument("Scalar text functions require TEXT columns: " +
                                expression.columnName);
  }
  return {expression.function, index};
}

std::string textFunctionName(TextFunction function) {
  switch (function) {
  case TextFunction::Upper:
    return "UPPER";
  case TextFunction::Lower:
    return "LOWER";
  case TextFunction::Length:
    return "LENGTH";
  }
  throw std::invalid_argument("Unknown scalar text function");
}

CompiledIntegerExpression
compileIntegerExpression(const IntegerExpression &expression,
                         const std::vector<ColumnDefinition> &schema,
                         const std::vector<SqlType> &types,
                         const std::string &tableQualifier) {
  CompiledIntegerExpression compiled;
  compiled.kind = expression.kind;
  if (expression.kind == IntegerExpression::Kind::Literal) {
    compiled.literal = expression.literal;
    return compiled;
  }
  if (expression.kind == IntegerExpression::Kind::Column) {
    const auto name =
        normalizeSingleTableColumnName(expression.columnName, tableQualifier);
    const auto found =
        std::find_if(schema.begin(), schema.end(),
                     [&](const auto &column) { return column.name == name; });
    if (found == schema.end()) {
      throw std::invalid_argument("Unknown SELECT column: " +
                                  expression.columnName);
    }
    const auto index =
        static_cast<std::size_t>(std::distance(schema.begin(), found));
    if (types[index] != SqlType::Int) {
      throw std::invalid_argument("Integer arithmetic requires INT columns: " +
                                  expression.columnName);
    }
    compiled.columnIndex = index;
    return compiled;
  }
  if (expression.kind == IntegerExpression::Kind::Unary) {
    if (!expression.operand) {
      throw std::invalid_argument("Malformed unary integer expression");
    }
    compiled.unaryOperation = expression.unaryOperation;
    compiled.operand =
        std::make_shared<CompiledIntegerExpression>(compileIntegerExpression(
            *expression.operand, schema, types, tableQualifier));
    return compiled;
  }
  if (!expression.left || !expression.right) {
    throw std::invalid_argument("Malformed integer arithmetic expression");
  }
  compiled.operation = expression.operation;
  compiled.left =
      std::make_shared<CompiledIntegerExpression>(compileIntegerExpression(
          *expression.left, schema, types, tableQualifier));
  compiled.right =
      std::make_shared<CompiledIntegerExpression>(compileIntegerExpression(
          *expression.right, schema, types, tableQualifier));
  return compiled;
}

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
