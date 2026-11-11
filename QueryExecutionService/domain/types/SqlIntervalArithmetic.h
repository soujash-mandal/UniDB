#pragma once

#include "SqlDateTimeValue.h"

namespace QueryExecutionService::domain {

/**
 * Add two PostgreSQL intervals component-wise.
 *
 * Months, days, and microseconds remain independent; months are never
 * converted into a fixed number of days.
 * @throws std::out_of_range if any result component overflows its type.
 */
SqlInterval addSqlIntervals(const SqlInterval &left, const SqlInterval &right);

/**
 * Subtract two PostgreSQL intervals component-wise.
 * @throws std::out_of_range if any result component overflows its type.
 */
SqlInterval subtractSqlIntervals(const SqlInterval &left,
                                 const SqlInterval &right);

/**
 * Negate every PostgreSQL interval component.
 * @throws std::out_of_range if any component is its type's minimum value.
 */
SqlInterval negateSqlInterval(const SqlInterval &interval);

} // namespace QueryExecutionService::domain
