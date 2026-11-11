#pragma once

#include "SqlDateTimeValue.h"

namespace QueryExecutionService::domain {

/**
 * Add a PostgreSQL INTERVAL to TIME WITH TIME ZONE, wrapping the local clock
 * within a 24-hour day while preserving the original UTC offset.
 * PostgreSQL ignores the interval's months and days components.
 */
SqlTimeWithTimeZone
addSqlTimeWithTimeZoneInterval(const SqlTimeWithTimeZone &time,
                               const SqlInterval &interval);

/**
 * Subtract a PostgreSQL INTERVAL from TIME WITH TIME ZONE, wrapping the local
 * clock within a 24-hour day while preserving the original UTC offset.
 * PostgreSQL ignores the interval's months and days components.
 */
SqlTimeWithTimeZone
subtractSqlTimeWithTimeZoneInterval(const SqlTimeWithTimeZone &time,
                                    const SqlInterval &interval);

} // namespace QueryExecutionService::domain
