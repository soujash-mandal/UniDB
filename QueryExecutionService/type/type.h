#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace unidb::sql {

// Canonical SQL type identities. Parser aliases resolve to these identities;
// aliases are not represented as separate runtime types.
enum class SqlTypeId : std::uint8_t {
  SmallInt,
  Integer,
  BigInt,
  Real,
  DoublePrecision,
  Numeric,
  Boolean,
  Char,
  Varchar,
  Text,
  Date,
  Time,
  TimeWithTimeZone,
  Timestamp,
  TimestampWithTimeZone,
  Interval,
  Bytea,
  Uuid,
  Json,
  Jsonb
};

// Type-specific declaration modifiers. The type-domain validator determines
// which fields are valid for each SqlTypeId and checks their PostgreSQL ranges.
struct SqlTypeModifier {
  std::optional<std::uint32_t> length;
  std::optional<std::uint32_t> precision;
  std::optional<std::int32_t> scale;
  std::optional<std::uint32_t> temporalPrecision;

  bool operator==(const SqlTypeModifier &) const = default;
};

// A declared SQL type is its canonical identity plus any validated modifiers.
// It is independent of runtime values and physical row encoding.
struct SqlTypeDescriptor {
  SqlTypeId id;
  SqlTypeModifier modifier;

  bool operator==(const SqlTypeDescriptor &) const = default;
};

// Transitional execution type used by the existing INT/TEXT codec and query
// implementation. It remains unchanged until each type family is integrated.
enum class SqlType { Int, Text };

using SqlIntegerValue = std::variant<std::int16_t, std::int32_t, std::int64_t>;

using SqlBooleanValue = bool;

using SqlRealValue = float;
using SqlDoublePrecisionValue = double;

// Exact finite decimal value. The unsigned coefficient stores decimal digits
// without leading or trailing zeroes (except canonical zero "0"); scale is
// the number of fractional digits and may be negative. This type is separate
// from SqlValue until decimal execution and storage are integrated.
struct SqlNumericValue {
  bool negative = false;
  std::string coefficient = "0";
  std::int32_t scale = 0;

  bool operator==(const SqlNumericValue &) const = default;
};

using SqlValue = std::variant<std::int64_t, std::string, SqlBooleanValue,
                              SqlRealValue, SqlDoublePrecisionValue,
                              SqlNumericValue>;

struct ColumnDefinition {
  std::string name;
  SqlType type;
  std::optional<SqlTypeDescriptor> declaredType;
};

} // namespace unidb::sql

namespace QueryExecutionService {

enum class QueryExecutionResultKind { Command, Rows };

struct QueryExecutionResult {
  QueryExecutionResultKind kind;
  std::string message;
  std::vector<unidb::sql::ColumnDefinition> columns;
  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  std::uint32_t affectedRows = 0;
};

enum class QueryExecutionErrorCode {
  SyntaxError,
  UnsupportedStatement,
  ExecutionError
};

struct QueryExecutionError {
  QueryExecutionErrorCode code;
  std::string message;
};

using QueryExecutionOutcome =
    std::variant<QueryExecutionResult, QueryExecutionError>;

struct ResolvedJoinColumn {
  const unidb::sql::ColumnDefinition *column;
  std::string tableName;
  std::size_t index;
};

} // namespace QueryExecutionService
