#include "SqlTimeArithmetic.h"

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

SqlTime addSqlTimeInterval(const SqlTime &time, const SqlInterval &interval) {
  validateSqlTime(time);
  const auto clock = wrapTimeOfDay(time.microsecondsSinceMidnight);
  const auto offset = wrapTimeOfDay(interval.microseconds);
  return SqlTime{wrapTimeOfDay(clock + offset)};
}

SqlTime addSqlTimeInterval(const SqlInterval &interval, const SqlTime &time) {
  return addSqlTimeInterval(time, interval);
}

SqlTime subtractSqlTimeInterval(const SqlTime &time,
                                const SqlInterval &interval) {
  validateSqlTime(time);
  const auto clock = wrapTimeOfDay(time.microsecondsSinceMidnight);
  const auto offset = wrapTimeOfDay(interval.microseconds);
  return SqlTime{wrapTimeOfDay(clock - offset)};
}

SqlInterval differenceSqlTimes(const SqlTime &left, const SqlTime &right) {
  validateSqlTime(left);
  validateSqlTime(right);
  return SqlInterval{
      0, 0, left.microsecondsSinceMidnight - right.microsecondsSinceMidnight};
}

} // namespace QueryExecutionService::domain
