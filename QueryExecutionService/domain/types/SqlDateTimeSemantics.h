#pragma once

#include "SqlDateTimeValue.h"

#include "../../type/type.h"

#include <cstdint>
#include <string_view>
#include <variant>

namespace QueryExecutionService::domain {

using SqlTemporalValue =
    std::variant<SqlDate, SqlTime, SqlTimeWithTimeZone, SqlTimestamp,
                 SqlTimestampWithTimeZone, SqlInterval>;

/**
 * Convert a SQL Gregorian date to days relative to the PostgreSQL epoch.
 * SQL year zero is rejected; negative years use 1 BC = year -1 numbering.
 * @throws std::invalid_argument for invalid calendar components.
 * @throws std::out_of_range when the resulting day count exceeds int32.
 */
SqlDate makeSqlDate(std::int32_t year, std::uint8_t month, std::uint8_t day);

/**
 * Validate a time-of-day, including PostgreSQL's 24:00:00 endpoint.
 * @throws std::out_of_range when the value falls outside the supported range.
 */
void validateSqlTime(const SqlTime &time);

/**
 * Validate a UTC displacement used by TIME WITH TIME ZONE.
 * @throws std::out_of_range when the offset exceeds the supported range.
 */
void validateSqlTimeZoneOffset(std::int32_t offsetSecondsEastOfUtc);

/**
 * Validate a timestamp's time component without changing its local meaning.
 * @throws std::out_of_range when the time component is outside its valid range.
 */
void validateSqlTimestamp(const SqlTimestamp &timestamp);

/**
 * Round fractional-second microseconds to the requested precision (0..6).
 * A result of 1,000,000 signals that the caller must carry into the next
 * second.
 * @throws std::out_of_range for invalid microsecond values.
 * @throws std::invalid_argument for precision greater than six.
 */
std::int64_t roundSqlFractionalSeconds(std::int64_t microseconds,
                                       std::uint8_t precision);

/** Parse and validate a typed SQL temporal literal using its declared
 * precision. */
SqlTemporalValue
parseSqlDateTimeLiteral(const unidb::sql::SqlTypeDescriptor &type,
                        std::string_view literal);

} // namespace QueryExecutionService::domain
