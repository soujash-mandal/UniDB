#include "SqlTimeWithTimeZoneArithmetic.h"

#include "SqlDateTimeSemantics.h"

#include <cstdint>

namespace QueryExecutionService::domain {
namespace {

constexpr std::int64_t kMicrosecondsPerDay = 86'400'000'000LL;

std::int64_t wrapTimeOfDay(std::int64_t microseconds) {
  const auto remainder = microseconds % kMicrosecondsPerDay;
  return remainder < 0 ? remainder + kMicrosecondsPerDay : remainder;
}

} // namespace

SqlTimeWithTimeZone addSqlTimeWithTimeZoneInterval(
    const SqlTimeWithTimeZone &time, const SqlInterval &interval) {
  validateSqlTime(time.time);
  validateSqlTimeZoneOffset(time.offsetSecondsEastOfUtc);

  const auto clock = wrapTimeOfDay(time.time.microsecondsSinceMidnight);
  const auto offset = wrapTimeOfDay(interval.microseconds);
  return SqlTimeWithTimeZone{
      SqlTime{wrapTimeOfDay(clock + offset)}, time.offsetSecondsEastOfUtc};
}

SqlTimeWithTimeZone subtractSqlTimeWithTimeZoneInterval(
    const SqlTimeWithTimeZone &time, const SqlInterval &interval) {
  validateSqlTime(time.time);
  validateSqlTimeZoneOffset(time.offsetSecondsEastOfUtc);

  const auto clock = wrapTimeOfDay(time.time.microsecondsSinceMidnight);
  const auto offset = wrapTimeOfDay(interval.microseconds);
  return SqlTimeWithTimeZone{
      SqlTime{wrapTimeOfDay(clock - offset)}, time.offsetSecondsEastOfUtc};
}

} // namespace QueryExecutionService::domain
