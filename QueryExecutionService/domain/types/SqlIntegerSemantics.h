#pragma once

#include "../../type/type.h"

#include <cstdint>
#include <variant>

namespace QueryExecutionService::domain {

// The runtime representation keeps the PostgreSQL integer widths distinct.
using unidb::sql::SqlIntegerValue;

// Returns false for every non-integer SQL type.
bool isIntegerSqlType(unidb::sql::SqlTypeId type);

// Returns the inclusive signed range for SMALLINT, INTEGER, or BIGINT.
// Throws std::invalid_argument for non-integer SQL types.
struct SqlIntegerBounds {
  std::int64_t minimum;
  std::int64_t maximum;
};

SqlIntegerBounds integerSqlTypeBounds(unidb::sql::SqlTypeId type);

// Builds a width-specific value and rejects values outside that SQL type's
// PostgreSQL range. Throws std::invalid_argument for non-integer types and
// std::out_of_range for values outside the selected type's range.
SqlIntegerValue makeSqlIntegerValue(unidb::sql::SqlTypeId type,
                                    std::int64_t value);

// Converts any supported integer alternative to the common comparison value.
std::int64_t integerValueAsInt64(const SqlIntegerValue &value);

} // namespace QueryExecutionService::domain
