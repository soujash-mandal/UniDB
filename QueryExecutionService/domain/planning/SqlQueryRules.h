#pragma once

#include "../../type/type.h"
#include "../contract/sqlStatement.h"
#include "../expression/ScalarExpressionEvaluator.h"

#include <string>
#include <vector>

namespace QueryExecutionService::domain {

using unidb::sql::ColumnDefinition;
using unidb::sql::IntegerExpression;
using unidb::sql::SelectStatement;
using unidb::sql::SqlType;
using unidb::sql::TextFunction;
using unidb::sql::TextFunctionExpression;
using unidb::sql::WhereExpression;

std::vector<SqlType> schemaTypes(const std::vector<ColumnDefinition> &schema);

ResolvedJoinColumn
resolveJoinColumn(const unidb::sql::ColumnReference &reference,
                  const std::string &leftTableName,
                  const std::vector<ColumnDefinition> &leftSchema,
                  const std::string &rightTableName,
                  const std::vector<ColumnDefinition> &rightSchema);

ResolvedJoinColumn
resolveSelectedJoinColumn(const std::string &name,
                          const std::string &leftTableName,
                          const std::vector<ColumnDefinition> &leftSchema,
                          const std::string &rightTableName,
                          const std::vector<ColumnDefinition> &rightSchema);

void validateInnerJoinColumns(const SelectStatement &select,
                              const std::vector<ColumnDefinition> &leftSchema,
                              const std::vector<ColumnDefinition> &rightSchema);

ResolvedJoinColumn
resolveWhereJoinColumn(const std::string &name,
                       const std::string &leftTableName,
                       const std::vector<ColumnDefinition> &leftSchema,
                       const std::string &rightTableName,
                       const std::vector<ColumnDefinition> &rightSchema);

std::string normalizeSingleTableColumnName(const std::string &name,
                                           const std::string &tableQualifier);

CompiledTextFunction
compileTextFunction(const TextFunctionExpression &expression,
                    const std::vector<ColumnDefinition> &schema,
                    const std::vector<SqlType> &types,
                    const std::string &tableQualifier);

std::string textFunctionName(TextFunction function);

CompiledIntegerExpression
compileIntegerExpression(const IntegerExpression &expression,
                         const std::vector<ColumnDefinition> &schema,
                         const std::vector<SqlType> &types,
                         const std::string &tableQualifier);

WhereExpression
normalizeSingleTableWhereColumns(const WhereExpression &expression,
                                 const std::string &tableName);

WhereExpression
qualifyJoinWhereColumns(const WhereExpression &expression,
                        const std::string &leftTableName,
                        const std::vector<ColumnDefinition> &leftSchema,
                        const std::string &rightTableName,
                        const std::vector<ColumnDefinition> &rightSchema);

} // namespace QueryExecutionService::domain
