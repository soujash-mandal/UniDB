#pragma once

#include "SqlDateTimeValue.h"

#include <cstdint>

namespace QueryExecutionService::domain {

/**
 * Add a number of calendar days to a DATE.
 * @throws std::out_of_range when the result is outside PostgreSQL's DATE range.
 */
SqlDate addSqlDateDays(SqlDate date, std::int32_t days);

/** Commutative form of DATE + INTEGER. */
SqlDate addSqlDateDays(std::int32_t days, SqlDate date);

/**
 * Subtract a number of calendar days from a DATE.
 * @throws std::out_of_range when the result is outside PostgreSQL's DATE range.
 */
SqlDate subtractSqlDateDays(SqlDate date, std::int32_t days);

/** Return the signed number of days from the second DATE to the first DATE. */
std::int32_t differenceSqlDates(SqlDate lhs, SqlDate rhs);

} // namespace QueryExecutionService::domain
