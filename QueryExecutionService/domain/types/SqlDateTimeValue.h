#pragma once

#include <cstdint>

namespace QueryExecutionService::domain {

// Calendar date represented as a signed number of days from 2000-01-01.
// Calendar conversion and range validation are deliberately implemented by
// later date/time semantics work, not by this storage-only value wrapper.
struct SqlDate {
  std::int32_t daysSinceEpoch = 0;

  bool operator==(const SqlDate &) const = default;
};

// Local time represented as microseconds since midnight. The semantic layer
// is responsible for validating the supported PostgreSQL time-of-day range.
struct SqlTime {
  std::int64_t microsecondsSinceMidnight = 0;

  bool operator==(const SqlTime &) const = default;
};

// TIME WITH TIME ZONE keeps the supplied numeric offset alongside the local
// clock reading. It is not an instant and has no date for DST calculations.
struct SqlTimeWithTimeZone {
  SqlTime time;
  std::int32_t offsetSecondsEastOfUtc = 0;

  bool operator==(const SqlTimeWithTimeZone &) const = default;
};

// TIMESTAMP WITHOUT TIME ZONE is a local date/time pair. It must never be
// implicitly normalized to UTC.
struct SqlTimestamp {
  SqlDate date;
  SqlTime time;

  bool operator==(const SqlTimestamp &) const = default;
};

// TIMESTAMP WITH TIME ZONE represents an absolute instant as microseconds
// from 2000-01-01T00:00:00Z. Session-timezone formatting is a separate concern.
struct SqlTimestampWithTimeZone {
  std::int64_t microsecondsSinceEpoch = 0;

  bool operator==(const SqlTimestampWithTimeZone &) const = default;
};

// INTERVAL preserves calendar months, calendar days, and sub-day time as
// independent components. In particular, months are not a fixed day count.
struct SqlInterval {
  std::int32_t months = 0;
  std::int32_t days = 0;
  std::int64_t microseconds = 0;

  bool operator==(const SqlInterval &) const = default;
};

} // namespace QueryExecutionService::domain
