#pragma once

#include "SqlDateTimeValue.h"

#include <cstdint>

namespace QueryExecutionService::domain {

// Convert a Gregorian calendar date to the signed day count used by SqlDate.
// Year zero is rejected; negative years use astronomical-free BC numbering.
SqlDate makeSqlDate(std::int32_t year, std::uint8_t month, std::uint8_t day);

// Validate a time-of-day. PostgreSQL permits exactly 24:00:00 as an endpoint.
void validateSqlTime(const SqlTime &time);

// Validate a numeric UTC displacement used by TIME WITH TIME ZONE.
void validateSqlTimeZoneOffset(std::int32_t offsetSecondsEastOfUtc);

// Validate the components of a timestamp without changing their local meaning.
void validateSqlTimestamp(const SqlTimestamp &timestamp);

// Round a fractional second to precision 0..6. A result of 1,000,000 means
// rounding carried into the next second; the caller advances the date/time.
std::int64_t roundSqlFractionalSeconds(std::int64_t microseconds,
                                       std::uint8_t precision);

} // namespace QueryExecutionService::domain
