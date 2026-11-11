#include "SqlDateTimeComparison.h"

#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace QueryExecutionService::domain {
namespace {

constexpr std::int64_t microsecondsPerSecond = 1'000'000;
constexpr std::int64_t microsecondsPerDay = 86'400 * microsecondsPerSecond;

std::int64_t utcTimeOfDay(const SqlTimeWithTimeZone &value) {
  const auto local = value.time.microsecondsSinceMidnight;
  const auto offset =
      static_cast<std::int64_t>(value.offsetSecondsEastOfUtc) *
      microsecondsPerSecond;
  auto utc = (local - offset) % microsecondsPerDay;
  if (utc < 0) {
    utc += microsecondsPerDay;
  }
  return utc;
}

std::pair<std::int64_t, std::int64_t>
normalizedInterval(const SqlInterval &value) {
  auto days = static_cast<std::int64_t>(value.months) * 30 + value.days;
  auto microseconds = value.microseconds;
  auto wholeDays = microseconds / microsecondsPerDay;
  auto remainder = microseconds % microsecondsPerDay;
  if (remainder < 0) {
    --wholeDays;
    remainder += microsecondsPerDay;
  }
  days += wholeDays;
  return {days, remainder};
}

template <typename T>
std::strong_ordering compare(const T &left, const T &right) {
  if (left < right) {
    return std::strong_ordering::less;
  }
  if (right < left) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

std::strong_ordering compare(const SqlTimeWithTimeZone &left,
                             const SqlTimeWithTimeZone &right) {
  return compare(utcTimeOfDay(left), utcTimeOfDay(right));
}

std::strong_ordering compare(const SqlTimestamp &left,
                             const SqlTimestamp &right) {
  if (const auto dateOrder =
          compare(left.date.daysSinceEpoch, right.date.daysSinceEpoch);
      dateOrder != std::strong_ordering::equal) {
    return dateOrder;
  }
  return compare(left.time.microsecondsSinceMidnight,
                 right.time.microsecondsSinceMidnight);
}

std::strong_ordering compare(const SqlInterval &left,
                             const SqlInterval &right) {
  return compare(normalizedInterval(left), normalizedInterval(right));
}

} // namespace

std::strong_ordering
compareSqlTemporalValues(const SqlTemporalValue &left,
                         const SqlTemporalValue &right) {
  if (left.index() != right.index()) {
    throw std::invalid_argument(
        "Cannot compare values with different temporal SQL types");
  }

  return std::visit(
      [](const auto &leftValue, const auto &rightValue) -> std::strong_ordering {
        using Left = std::decay_t<decltype(leftValue)>;
        using Right = std::decay_t<decltype(rightValue)>;
        if constexpr (!std::is_same_v<Left, Right>) {
          throw std::invalid_argument(
              "Cannot compare values with different temporal SQL types");
        } else {
          return compare(leftValue, rightValue);
        }
      },
      left, right);
}

} // namespace QueryExecutionService::domain
