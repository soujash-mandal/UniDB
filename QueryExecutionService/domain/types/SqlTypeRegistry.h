#pragma once

#include "../../type/type.h"

#include <string_view>

namespace QueryExecutionService::domain {

// Resolve PostgreSQL spellings and aliases to one canonical SQL type
// descriptor. Invalid names or modifiers throw std::invalid_argument.
unidb::sql::SqlTypeDescriptor
resolveSqlType(std::string_view name,
               unidb::sql::SqlTypeModifier modifier = {});

std::string_view canonicalSqlTypeName(unidb::sql::SqlTypeId type);

} // namespace QueryExecutionService::domain
