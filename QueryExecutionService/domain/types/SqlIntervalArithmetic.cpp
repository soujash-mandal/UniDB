#include "SqlIntervalArithmetic.h"

#include <limits>
#include <stdexcept>

namespace QueryExecutionService::domain {
namespace {

template <typename T> T checkedAdd(T left, T right) {
  constexpr auto minimum = std::numeric_limits<T>::min();
  constexpr auto maximum = std::numeric_limits<T>::max();

  if ((right > 0 && left > maximum - right) ||
      (right < 0 && left < minimum - right)) {
    throw std::out_of_range("SQL INTERVAL component overflow");
  }
  return static_cast<T>(left + right);
}

template <typename T> T checkedSubtract(T left, T right) {
  constexpr auto minimum = std::numeric_limits<T>::min();
  constexpr auto maximum = std::numeric_limits<T>::max();

  if ((right > 0 && left < minimum + right) ||
      (right < 0 && left > maximum + right)) {
    throw std::out_of_range("SQL INTERVAL component overflow");
  }
  return static_cast<T>(left - right);
}

template <typename T> T checkedNegate(T value) {
  if (value == std::numeric_limits<T>::min()) {
    throw std::out_of_range("SQL INTERVAL component overflow");
  }
  return static_cast<T>(-value);
}

} // namespace

SqlInterval addSqlIntervals(const SqlInterval &left, const SqlInterval &right) {
  return SqlInterval{checkedAdd(left.months, right.months),
                     checkedAdd(left.days, right.days),
                     checkedAdd(left.microseconds, right.microseconds)};
}

SqlInterval subtractSqlIntervals(const SqlInterval &left,
                                 const SqlInterval &right) {
  return SqlInterval{checkedSubtract(left.months, right.months),
                     checkedSubtract(left.days, right.days),
                     checkedSubtract(left.microseconds, right.microseconds)};
}

SqlInterval negateSqlInterval(const SqlInterval &interval) {
  return SqlInterval{checkedNegate(interval.months),
                     checkedNegate(interval.days),
                     checkedNegate(interval.microseconds)};
}

} // namespace QueryExecutionService::domain
