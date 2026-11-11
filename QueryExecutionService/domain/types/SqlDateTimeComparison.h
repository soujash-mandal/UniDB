#pragma once

#include "SqlDateTimeSemantics.h"

#include <compare>

namespace QueryExecutionService::domain {

// Compare values only when they have the same temporal SQL type.
// TIMESTAMP is local; TIMESTAMPTZ is an absolute instant.
std::strong_ordering compareSqlTemporalValues(const SqlTemporalValue &left,
                                              const SqlTemporalValue &right);

} // namespace QueryExecutionService::domain
