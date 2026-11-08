#pragma once

#include "ScalarExpressionEvaluator.h"

#include <string>
#include <vector>

namespace QueryExecutionService::domain {

CompiledTextFunction
compileTextFunction(const unidb::sql::TextFunctionExpression &expression,
                    const std::vector<unidb::sql::ColumnDefinition> &schema,
                    const std::vector<unidb::sql::SqlType> &types,
                    const std::string &tableQualifier);

std::string textFunctionName(unidb::sql::TextFunction function);

CompiledIntegerExpression compileIntegerExpression(
    const unidb::sql::IntegerExpression &expression,
    const std::vector<unidb::sql::ColumnDefinition> &schema,
    const std::vector<unidb::sql::SqlType> &types,
    const std::string &tableQualifier);

} // namespace QueryExecutionService::domain
