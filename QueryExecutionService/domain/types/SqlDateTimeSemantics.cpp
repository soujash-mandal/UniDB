#include "SqlDateTimeSemantics.h"

#include <array>
#include <cmath>
#include <cctype>
#include <limits>
#include <sstream>
#include <string>
#include <limits>
#include <stdexcept>

namespace QueryExecutionService::domain {
namespace {

bool isLeapYear(std::int64_t year) {
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

  // SQL BC years omit year zero; map them to astronomical numbering.
  const std::int64_t astronomicalYear =
      year < 0 ? std::int64_t{year} + 1 : year;

  auto maximumDay = daysInMonth[month - 1];
  if (month == 2 && isLeapYear(astronomicalYear)) {
    ++maximumDay;
  }
  if (day < 1 || day > maximumDay) {
    throw std::invalid_argument("Invalid SQL calendar date");
  }

  // Howard Hinnant's proleptic-Gregorian civil-date conversion.
  std::int64_t adjustedYear = astronomicalYear;
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
  const auto result = daysSinceUnixEpoch - postgresEpochFromUnixEpoch;

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


namespace {

constexpr std::int64_t microsecondsPerSecond = 1'000'000;
constexpr std::int64_t microsecondsPerDay = 86'400 * microsecondsPerSecond;

int parseDigits(std::string_view value, std::size_t position,
                std::size_t count) {
  if (position + count > value.size()) {
    throw std::invalid_argument("Incomplete SQL date/time literal");
  }
  int result = 0;
  for (std::size_t index = position; index < position + count; ++index) {
    const char character = value[index];
    if (character < '0' || character > '9') {
      throw std::invalid_argument("Invalid SQL date/time literal");
    }
    result = result * 10 + character - '0';
  }
  return result;
}

SqlDate parseDateLiteral(std::string_view value) {
  if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
    throw std::invalid_argument("DATE literal must use YYYY-MM-DD");
  }
  const auto year = parseDigits(value, 0, 4);
  const auto month = parseDigits(value, 5, 2);
  const auto day = parseDigits(value, 8, 2);
  return makeSqlDate(year, static_cast<std::uint8_t>(month),
                     static_cast<std::uint8_t>(day));
}

std::int32_t parseOffset(std::string_view value) {
  if (value.size() != 6 && value.size() != 9) {
    throw std::invalid_argument("Time zone offset must be +HH:MM or +HH:MM:SS");
  }
  if (value[0] != '+' && value[0] != '-') {
    throw std::invalid_argument("Invalid time zone offset sign");
  }
  if (value[3] != ':' || (value.size() == 9 && value[6] != ':')) {
    throw std::invalid_argument("Invalid time zone offset");
  }
  const int hours = parseDigits(value, 1, 2);
  const int minutes = parseDigits(value, 4, 2);
  const int seconds = value.size() == 9 ? parseDigits(value, 7, 2) : 0;
  if (minutes > 59 || seconds > 59) {
    throw std::invalid_argument("Invalid time zone offset");
  }
  const int magnitude = hours * 3600 + minutes * 60 + seconds;
  const int offset = value[0] == '-' ? -magnitude : magnitude;
  validateSqlTimeZoneOffset(offset);
  return offset;
}

struct ParsedClock {
  SqlTime time;
  std::int32_t offsetSecondsEastOfUtc = 0;
  bool hasOffset = false;
};

ParsedClock parseClock(std::string_view value, std::uint8_t precision,
                       bool offsetAllowed, bool offsetRequired) {
  std::size_t offsetPosition = std::string_view::npos;
  for (std::size_t index = 8; index < value.size(); ++index) {
    if (value[index] == '+' || value[index] == '-') {
      offsetPosition = index;
      break;
    }
  }

  const bool hasOffset = offsetPosition != std::string_view::npos;
  if (hasOffset && !offsetAllowed) {
    throw std::invalid_argument(
        "A local TIME/TIMESTAMP literal cannot contain a time zone offset");
  }
  if (offsetRequired && !hasOffset) {
    throw std::invalid_argument("A time zone offset is required for this type");
  }

  const auto clock = hasOffset ? value.substr(0, offsetPosition) : value;
  const auto offset = hasOffset ? parseOffset(value.substr(offsetPosition)) : 0;
  if (clock.size() < 8 || clock[2] != ':' || clock[5] != ':') {
    throw std::invalid_argument("Time literal must use HH:MM:SS");
  }

  const int hours = parseDigits(clock, 0, 2);
  const int minutes = parseDigits(clock, 3, 2);
  const int seconds = parseDigits(clock, 6, 2);
  if (minutes > 59 || seconds > 59 || hours > 24 ||
      (hours == 24 && (minutes != 0 || seconds != 0))) {
    throw std::invalid_argument("Invalid SQL time of day");
  }

  std::int64_t fractionalMicroseconds = 0;
  if (clock.size() > 8) {
    if (clock[8] != '.' || clock.size() == 9 || clock.size() > 15) {
      throw std::invalid_argument("Invalid fractional seconds");
    }
    const auto digits = clock.substr(9);
    fractionalMicroseconds =
        parseDigits(digits, 0, digits.size()) *
        (digits.size() == 1 ? 100000 :
         digits.size() == 2 ? 10000 :
         digits.size() == 3 ? 1000 :
         digits.size() == 4 ? 100 :
         digits.size() == 5 ? 10 : 1);
  }
  if (hours == 24 && fractionalMicroseconds != 0) {
    throw std::invalid_argument("24:00:00 cannot have fractional seconds");
  }

  auto rounded = roundSqlFractionalSeconds(fractionalMicroseconds, precision);
  std::int64_t wholeSeconds = hours * 3600 + minutes * 60 + seconds;
  if (rounded == microsecondsPerSecond) {
    ++wholeSeconds;
    rounded = 0;
  }
  const auto total = wholeSeconds * microsecondsPerSecond + rounded;
  SqlTime time{total};
  validateSqlTime(time);
  return ParsedClock{time, offset, hasOffset};
}

SqlDate incrementDate(SqlDate date) {
  if (date.daysSinceEpoch == std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("Timestamp is outside the supported date range");
  }
  ++date.daysSinceEpoch;
  return date;
}

SqlTimestamp parseTimestampLiteral(std::string_view value,
                                   std::uint8_t precision,
                                   bool withTimeZone) {
  if (value.size() < 19 || (value[10] != ' ' && value[10] != 'T')) {
    throw std::invalid_argument(
        "TIMESTAMP literal must use YYYY-MM-DD HH:MM:SS");
  }
  auto date = parseDateLiteral(value.substr(0, 10));
  auto clock = parseClock(value.substr(11), precision, withTimeZone,
                          withTimeZone);
  if (clock.time.microsecondsSinceMidnight == microsecondsPerDay) {
    date = incrementDate(date);
    clock.time.microsecondsSinceMidnight = 0;
  }
  return SqlTimestamp{date, clock.time};
}

SqlInterval parseIntervalLiteral(std::string_view value) {
  std::istringstream input{std::string(value)};
  long double amount = 0;
  std::string unit;
  std::int64_t months = 0;
  std::int64_t days = 0;
  long double microseconds = 0;
  bool found = false;

  while (input >> amount >> unit) {
    found = true;
    for (char &character : unit) {
      character = static_cast<char>(
          std::tolower(static_cast<unsigned char>(character)));
    }
    if (unit == "year" || unit == "years" || unit == "yr" ||
        unit == "yrs") {
      if (std::floor(amount) != amount) {
        throw std::invalid_argument("INTERVAL years must be whole numbers");
      }
      months += static_cast<std::int64_t>(amount) * 12;
    } else if (unit == "mon" || unit == "mons" || unit == "month" ||
               unit == "months") {
      if (std::floor(amount) != amount) {
        throw std::invalid_argument("INTERVAL months must be whole numbers");
      }
      months += static_cast<std::int64_t>(amount);
    } else if (unit == "day" || unit == "days") {
      if (std::floor(amount) != amount) {
        throw std::invalid_argument("INTERVAL days must be whole numbers");
      }
      days += static_cast<std::int64_t>(amount);
    } else if (unit == "hour" || unit == "hours" || unit == "hr" ||
               unit == "hrs") {
      microseconds += amount * 3'600 * microsecondsPerSecond;
    } else if (unit == "minute" || unit == "minutes" || unit == "min" ||
               unit == "mins") {
      microseconds += amount * 60 * microsecondsPerSecond;
    } else if (unit == "second" || unit == "seconds" || unit == "sec" ||
               unit == "secs") {
      microseconds += amount * microsecondsPerSecond;
    } else if (unit == "millisecond" || unit == "milliseconds" ||
               unit == "msec" || unit == "msecs") {
      microseconds += amount * 1000;
    } else if (unit == "microsecond" || unit == "microseconds" ||
               unit == "usec" || unit == "usecs") {
      microseconds += amount;
    } else {
      throw std::invalid_argument("Unsupported INTERVAL field: " + unit);
    }
  }

  if (!found || !input.eof()) {
    throw std::invalid_argument(
        "INTERVAL literal must contain number/unit field pairs");
  }
  const auto roundedMicros = std::llround(microseconds);
  if (months < std::numeric_limits<std::int32_t>::min() ||
      months > std::numeric_limits<std::int32_t>::max() ||
      days < std::numeric_limits<std::int32_t>::min() ||
      days > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("INTERVAL is outside the supported range");
  }
  return SqlInterval{static_cast<std::int32_t>(months),
                     static_cast<std::int32_t>(days), roundedMicros};
}

} // namespace

SqlTemporalValue
parseSqlDateTimeLiteral(const unidb::sql::SqlTypeDescriptor &type,
                        std::string_view literal) {
  const auto precision = static_cast<std::uint8_t>(
      type.modifier.temporalPrecision.value_or(6));
  switch (type.id) {
  case unidb::sql::SqlTypeId::Date:
    return parseDateLiteral(literal);
  case unidb::sql::SqlTypeId::Time: {
    const auto parsed = parseClock(literal, precision, false, false);
    return parsed.time;
  }
  case unidb::sql::SqlTypeId::TimeWithTimeZone: {
    const auto parsed = parseClock(literal, precision, true, true);
    return SqlTimeWithTimeZone{parsed.time, parsed.offsetSecondsEastOfUtc};
  }
  case unidb::sql::SqlTypeId::Timestamp:
    return parseTimestampLiteral(literal, precision, false);
  case unidb::sql::SqlTypeId::TimestampWithTimeZone: {
    const auto local = parseTimestampLiteral(literal, precision, true);
    const auto offsetPosition = literal.find_last_of("+-");
    if (offsetPosition == std::string_view::npos || offsetPosition < 19) {
      throw std::invalid_argument("TIMESTAMPTZ requires an explicit UTC offset");
    }
    const auto offset = parseOffset(literal.substr(offsetPosition));
    const auto dayMicros =
        static_cast<std::int64_t>(local.date.daysSinceEpoch) *
        microsecondsPerDay;
    const auto localMicros = dayMicros +
                             local.time.microsecondsSinceMidnight;
    if ((offset > 0 && localMicros <
                           std::numeric_limits<std::int64_t>::min() +
                               static_cast<std::int64_t>(offset) *
                                   microsecondsPerSecond) ||
        (offset < 0 && localMicros >
                           std::numeric_limits<std::int64_t>::max() +
                               static_cast<std::int64_t>(offset) *
                                   microsecondsPerSecond)) {
      throw std::out_of_range("TIMESTAMPTZ is outside the supported range");
    }
    return SqlTimestampWithTimeZone{
        localMicros - static_cast<std::int64_t>(offset) *
                          microsecondsPerSecond};
  }
  case unidb::sql::SqlTypeId::Interval:
    return parseIntervalLiteral(literal);
  default:
    throw std::invalid_argument("Type is not a supported temporal literal");
  }
}

} // namespace QueryExecutionService::domain
