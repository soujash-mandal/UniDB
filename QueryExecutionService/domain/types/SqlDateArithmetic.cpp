#include "SqlDateArithmetic.h"

#include <limits>
#include <stdexcept>

namespace QueryExecutionService::domain {
namespace {

// PostgreSQL's finite DATE bounds, expressed as days from 2000-01-01.
constexpr std::int64_t minimumPostgresDate = -2'451'545;
constexpr std::int64_t maximumPostgresDate = 2'145'031'948;

SqlDate checkedSqlDate(std::int64_t daysSinceEpoch) {
  if (daysSinceEpoch < minimumPostgresDate ||
      daysSinceEpoch > maximumPostgresDate) {
    throw std::out_of_range("DATE arithmetic result is outside PostgreSQL's supported range");
  }
  return SqlDate{static_cast<std::int32_t>(daysSinceEpoch)};
}

} // namespace

SqlDate addSqlDateDays(SqlDate date, std::int32_t days) {
  return checkedSqlDate(static_cast<std::int64_t>(date.daysSinceEpoch) + days);
}

SqlDate addSqlDateDays(std::int32_t days, SqlDate date) {
  return addSqlDateDays(date, days);
}

SqlDate subtractSqlDateDays(SqlDate date, std::int32_t days) {
  return checkedSqlDate(static_cast<std::int64_t>(date.daysSinceEpoch) - days);
}

std::int32_t differenceSqlDates(SqlDate lhs, SqlDate rhs) {
  const auto difference = static_cast<std::int64_t>(lhs.daysSinceEpoch) -
                          static_cast<std::int64_t>(rhs.daysSinceEpoch);
  if (difference < std::numeric_limits<std::int32_t>::min() ||
      difference > std::numeric_limits<std::int32_t>::max()) {
    throw std::out_of_range("DATE difference exceeds PostgreSQL's integer range");
  }
  return static_cast<std::int32_t>(difference);
}

} // namespace QueryExecutionService::domain
