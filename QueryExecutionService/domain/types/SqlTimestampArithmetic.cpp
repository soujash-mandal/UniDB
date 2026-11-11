#include "SqlTimestampArithmetic.h"

#include "SqlDateTimeSemantics.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace QueryExecutionService::domain {
namespace {

constexpr std::int64_t kMicrosecondsPerDay = 86'400'000'000LL;
constexpr std::int64_t kPostgresEpochToUnixEpochDays = 10'957;
constexpr std::int64_t kMinimumTimestamp = -211'813'488'000'000'000LL;
constexpr std::int64_t kEndTimestamp = 9'223'371'331'200'000'000LL;

struct CivilDate {
  std::int64_t year;
  std::int32_t month;
  std::int32_t day;
};

constexpr std::array<std::int32_t, 12> kDaysInMonth{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

bool isLeapYear(std::int64_t year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

std::int64_t floorDivide(std::int64_t value, std::int64_t divisor) {
  const auto quotient = value / divisor;
  const auto remainder = value % divisor;
  return remainder < 0 ? quotient - 1 : quotient;
}

std::int64_t positiveRemainder(std::int64_t value, std::int64_t divisor) {
  const auto remainder = value % divisor;
  return remainder < 0 ? remainder + divisor : remainder;
}

CivilDate civilDateFromPostgresDays(std::int64_t days) {
  auto z = days + kPostgresEpochToUnixEpochDays + 719'468;
  const auto era = floorDivide(z, 146'097);
  const auto dayOfEra = z - era * 146'097;
  const auto yearOfEra =
      (dayOfEra - dayOfEra / 1'460 + dayOfEra / 36'524 -
       dayOfEra / 146'096) /
      365;
  auto year = yearOfEra + era * 400;
  const auto dayOfYear =
      dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
  const auto monthPrime = (5 * dayOfYear + 2) / 153;
  const auto day = dayOfYear - (153 * monthPrime + 2) / 5 + 1;
  const auto month = monthPrime + (monthPrime < 10 ? 3 : -9);
  year += month <= 2;
  return CivilDate{year, static_cast<std::int32_t>(month),
                   static_cast<std::int32_t>(day)};
}

std::int32_t daysInMonth(std::int64_t year, std::int32_t month) {
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return kDaysInMonth[static_cast<std::size_t>(month - 1)];
}

std::int64_t timestampToMicroseconds(const SqlTimestamp &timestamp) {
  validateSqlTimestamp(timestamp);
  auto day = static_cast<std::int64_t>(timestamp.date.daysSinceEpoch);
  auto time = timestamp.time.microsecondsSinceMidnight;
  if (time == kMicrosecondsPerDay) {
    ++day;
    time = 0;
  }

  const auto minimumDay = floorDivide(kMinimumTimestamp, kMicrosecondsPerDay);
  const auto maximumDay =
      floorDivide(kEndTimestamp - 1, kMicrosecondsPerDay);
  if (day < minimumDay || day > maximumDay) {
    throw std::out_of_range("SQL timestamp is outside PostgreSQL's range");
  }

  const auto dayMicroseconds = day * kMicrosecondsPerDay;
  if (dayMicroseconds > std::numeric_limits<std::int64_t>::max() - time) {
    throw std::out_of_range("SQL timestamp is outside PostgreSQL's range");
  }
  const auto result = dayMicroseconds + time;
  if (result < kMinimumTimestamp || result >= kEndTimestamp) {
    throw std::out_of_range("SQL timestamp is outside PostgreSQL's range");
  }
  return result;
}

SqlTimestamp timestampFromMicroseconds(std::int64_t value) {
  if (value < kMinimumTimestamp || value >= kEndTimestamp) {
    throw std::out_of_range("SQL timestamp is outside PostgreSQL's range");
  }
  const auto day = floorDivide(value, kMicrosecondsPerDay);
  const auto time = value - day * kMicrosecondsPerDay;
  if (day < std::numeric_limits<std::int32_t>::min() ||
      day > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("SQL timestamp date is outside its representation");
  }
  return SqlTimestamp{SqlDate{static_cast<std::int32_t>(day)},
                      SqlTime{time}};
}

SqlDate addCalendarMonths(SqlDate date, std::int32_t months) {
  const auto civil = civilDateFromPostgresDays(date.daysSinceEpoch);
  const auto currentMonthIndex = civil.year * 12 + civil.month - 1;
  if ((months > 0 &&
       currentMonthIndex >
           std::numeric_limits<std::int64_t>::max() - months) ||
      (months < 0 &&
       currentMonthIndex <
           std::numeric_limits<std::int64_t>::min() - months)) {
    throw std::out_of_range("SQL timestamp month arithmetic overflow");
  }

  const auto targetMonthIndex = currentMonthIndex + months;
  const auto targetYear = floorDivide(targetMonthIndex, 12);
  const auto targetMonth =
      static_cast<std::int32_t>(positiveRemainder(targetMonthIndex, 12) + 1);
  const auto targetDay =
      std::min(civil.day, daysInMonth(targetYear, targetMonth));

  const auto sqlYear =
      targetYear <= 0 ? targetYear - 1 : targetYear;
  if (sqlYear < std::numeric_limits<std::int32_t>::min() ||
      sqlYear > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("SQL timestamp year is outside its representation");
  }
  return makeSqlDate(static_cast<std::int32_t>(sqlYear),
                     static_cast<std::uint8_t>(targetMonth),
                     static_cast<std::uint8_t>(targetDay));
}

SqlDate addCalendarDays(SqlDate date, std::int32_t days) {
  const auto result = static_cast<std::int64_t>(date.daysSinceEpoch) + days;
  if (result < std::numeric_limits<std::int32_t>::min() ||
      result > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("SQL timestamp day arithmetic overflow");
  }
  return SqlDate{static_cast<std::int32_t>(result)};
}

std::int64_t checkedNegate(std::int64_t value) {
  if (value == std::numeric_limits<std::int64_t>::min()) {
    throw std::out_of_range("SQL timestamp interval subtraction overflow");
  }
  return -value;
}

std::int32_t checkedNegate(std::int32_t value) {
  if (value == std::numeric_limits<std::int32_t>::min()) {
    throw std::out_of_range("SQL timestamp interval subtraction overflow");
  }
  return -value;
}

SqlTimestamp applyInterval(const SqlTimestamp &timestamp,
                           const SqlInterval &interval, bool subtract) {
  auto result = timestamp;
  timestampToMicroseconds(result);

  const auto months =
      subtract ? checkedNegate(interval.months) : interval.months;
  const auto days = subtract ? checkedNegate(interval.days) : interval.days;
  const auto microseconds =
      subtract ? checkedNegate(interval.microseconds) : interval.microseconds;

  if (months != 0) {
    result.date = addCalendarMonths(result.date, months);
  }
  if (days != 0) {
    result.date = addCalendarDays(result.date, days);
  }

  const auto current = timestampToMicroseconds(result);
  if ((microseconds > 0 &&
       current > std::numeric_limits<std::int64_t>::max() - microseconds) ||
      (microseconds < 0 &&
       current < std::numeric_limits<std::int64_t>::min() - microseconds)) {
    throw std::out_of_range("SQL timestamp time arithmetic overflow");
  }
  return timestampFromMicroseconds(current + microseconds);
}

} // namespace

SqlTimestamp addSqlTimestampInterval(const SqlTimestamp &timestamp,
                                     const SqlInterval &interval) {
  return applyInterval(timestamp, interval, false);
}

SqlTimestamp addSqlTimestampInterval(const SqlInterval &interval,
                                     const SqlTimestamp &timestamp) {
  return addSqlTimestampInterval(timestamp, interval);
}

SqlTimestamp subtractSqlTimestampInterval(const SqlTimestamp &timestamp,
                                          const SqlInterval &interval) {
  return applyInterval(timestamp, interval, true);
}

SqlInterval differenceSqlTimestamps(const SqlTimestamp &left,
                                    const SqlTimestamp &right) {
  const auto leftMicroseconds = timestampToMicroseconds(left);
  const auto rightMicroseconds = timestampToMicroseconds(right);
  if ((rightMicroseconds < 0 &&
       leftMicroseconds >
           std::numeric_limits<std::int64_t>::max() + rightMicroseconds) ||
      (rightMicroseconds > 0 &&
       leftMicroseconds <
           std::numeric_limits<std::int64_t>::min() + rightMicroseconds)) {
    throw std::out_of_range("SQL timestamp difference overflow");
  }
  return SqlInterval{0, 0, leftMicroseconds - rightMicroseconds};
}

} // namespace QueryExecutionService::domain
