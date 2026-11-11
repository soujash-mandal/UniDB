#pragma once

#include "SqlDateTimeValue.h"

namespace QueryExecutionService::domain {

/**
 * Add a PostgreSQL INTERVAL to TIME, wrapping within a 24-hour day.
 * PostgreSQL ignores the interval's months and days components for TIME
 * arithmetic; only its microseconds component affects the clock value.
 */
SqlTime addSqlTimeInterval(const SqlTime &time, const SqlInterval &interval);

/** Commutative overload for INTERVAL + TIME. */
SqlTime addSqlTimeInterval(const SqlInterval &interval, const SqlTime &time);

/**
 * Subtract a PostgreSQL INTERVAL from TIME, wrapping within a 24-hour day.
 * The interval's months and days components are ignored.
 */
SqlTime subtractSqlTimeInterval(const SqlTime &time,
                                const SqlInterval &interval);

/**
 * Subtract two TIME values, returning an interval with zero months and days.
 */
SqlInterval differenceSqlTimes(const SqlTime &left, const SqlTime &right);

} // namespace QueryExecutionService::domain
