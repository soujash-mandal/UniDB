#pragma once

#include "SqlDateTimeValue.h"

namespace QueryExecutionService::domain {

/**
 * Add a PostgreSQL INTERVAL to TIMESTAMP WITHOUT TIME ZONE.
 * Calendar months clamp to the final day of the target month; days preserve
 * local time; the microseconds component is then added as elapsed local time.
 */
SqlTimestamp addSqlTimestampInterval(const SqlTimestamp &timestamp,
                                     const SqlInterval &interval);

/** Commutative overload for INTERVAL + TIMESTAMP. */
SqlTimestamp addSqlTimestampInterval(const SqlInterval &interval,
                                     const SqlTimestamp &timestamp);

/** Subtract a PostgreSQL INTERVAL from TIMESTAMP WITHOUT TIME ZONE. */
SqlTimestamp subtractSqlTimestampInterval(const SqlTimestamp &timestamp,
                                          const SqlInterval &interval);

/**
 * Subtract two local timestamps. The result has zero months and days, and
 * stores the signed difference in microseconds.
 */
SqlInterval differenceSqlTimestamps(const SqlTimestamp &left,
                                    const SqlTimestamp &right);

} // namespace QueryExecutionService::domain
