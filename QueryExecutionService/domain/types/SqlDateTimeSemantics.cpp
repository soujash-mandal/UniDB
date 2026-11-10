#include "SqlDateTimeSemantics.h"

#include <array>
#include <limits>
#include <stdexcept>

namespace QueryExecutionService::domain {
namespace {

bool isLeapYear(std::int32_t year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

constexpr std::array<std::uint8_t, 12> daysInMonth{31, 28, 31, 30, 31, 30,
                                                   31, 31, 30, 31, 30, 31};

constexpr std::int64_t microsecondsPerSecond = 1'000'000;
constexpr std::int64_t microsecondsPerDay = 86'400 * microsecondsPerSecond;

} // namespace

SqlDate makeSqlDate(std::int32_t year, std::uint8_t month, std::uint8_t day) {
  if (year == 0 || month < 1 || month > 12) {
    throw std::invalid_argument("Invalid SQL calendar date");
  }

  auto maximumDay = daysInMonth[month - 1];
  if (month == 2 && isLeapYear(year)) {
    ++maximumDay;
  }
  if (day < 1 || day > maximumDay) {
    throw std::invalid_argument("Invalid SQL calendar date");
  }

  // Howard Hinnant's proleptic-Gregorian civil-date conversion.
  std::int64_t adjustedYear = year;
  adjustedYear -= month <= 2;
  const auto era =
      (adjustedYear >= 0 ? adjustedYear : adjustedYear - 399) / 400;
  const auto yearOfEra = adjustedYear - era * 400;
  const auto adjustedMonth =
      static_cast<std::int32_t>(month) + (month > 2 ? -3 : 9);
  const auto dayOfYear =
      (153 * adjustedMonth + 2) / 5 + static_cast<std::int32_t>(day) - 1;
  const auto dayOfEra =
      yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  const auto daysSinceUnixEpoch = era * 146097 + dayOfEra - 719468;
  constexpr std::int64_t postgresEpochFromUnixEpoch = 10957;
  const auto result = daysSinceUnixEpoch + postgresEpochFromUnixEpoch;

  if (result < std::numeric_limits<std::int32_t>::min() ||
      result > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("SQL date is outside the supported representation");
  }
  return SqlDate{static_cast<std::int32_t>(result)};
}

void validateSqlTime(const SqlTime &time) {
  if (time.microsecondsSinceMidnight < 0 ||
      time.microsecondsSinceMidnight > microsecondsPerDay) {
    throw std::out_of_range("SQL time is outside 00:00:00 through 24:00:00");
  }
}

void validateSqlTimeZoneOffset(std::int32_t offsetSecondsEastOfUtc) {
  constexpr std::int32_t maximumOffsetSeconds = 15 * 3600 + 59 * 60 + 59;
  if (offsetSecondsEastOfUtc < -maximumOffsetSeconds ||
      offsetSecondsEastOfUtc > maximumOffsetSeconds) {
    throw std::out_of_range("SQL time zone offset exceeds 15:59:59");
  }
}

void validateSqlTimestamp(const SqlTimestamp &timestamp) {
  validateSqlTime(timestamp.time);
}

std::int64_t roundSqlFractionalSeconds(std::int64_t microseconds,
                                       std::uint8_t precision) {
  if (microseconds < 0 || microseconds >= microsecondsPerSecond) {
    throw std::out_of_range("Fractional seconds must be between 0 and 999999");
  }
  if (precision > 6) {
    throw std::invalid_argument(
        "SQL temporal precision must be between 0 and 6");
  }

  std::int64_t scale = 1;
  for (auto digits = precision; digits < 6; ++digits) {
    scale *= 10;
  }
  return ((microseconds + scale / 2) / scale) * scale;
}

} // namespace QueryExecutionService::domain
