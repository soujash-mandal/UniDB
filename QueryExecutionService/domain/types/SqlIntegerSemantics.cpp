#include "SqlIntegerSemantics.h"

#include <limits>
#include <stdexcept>

namespace QueryExecutionService::domain {

bool isIntegerSqlType(unidb::sql::SqlTypeId type) {
  using unidb::sql::SqlTypeId;
  return type == SqlTypeId::SmallInt || type == SqlTypeId::Integer ||
         type == SqlTypeId::BigInt;
}

SqlIntegerBounds integerSqlTypeBounds(unidb::sql::SqlTypeId type) {
  using unidb::sql::SqlTypeId;
  switch (type) {
  case SqlTypeId::SmallInt:
    return {std::numeric_limits<std::int16_t>::min(),
            std::numeric_limits<std::int16_t>::max()};
  case SqlTypeId::Integer:
    return {std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max()};
  case SqlTypeId::BigInt:
    return {std::numeric_limits<std::int64_t>::min(),
            std::numeric_limits<std::int64_t>::max()};
  default:
    throw std::invalid_argument("SQL type is not an integer type");
  }
}

SqlIntegerValue makeSqlIntegerValue(unidb::sql::SqlTypeId type,
                                    std::int64_t value) {
  const auto bounds = integerSqlTypeBounds(type);
  if (value < bounds.minimum || value > bounds.maximum) {
    switch (type) {
    case unidb::sql::SqlTypeId::SmallInt:
      throw std::out_of_range("SMALLINT value is out of range");
    case unidb::sql::SqlTypeId::Integer:
      throw std::out_of_range("INTEGER value is out of range");
    case unidb::sql::SqlTypeId::BigInt:
      throw std::out_of_range("BIGINT value is out of range");
    default:
      break;
    }
  }

  switch (type) {
  case unidb::sql::SqlTypeId::SmallInt:
    return static_cast<std::int16_t>(value);
  case unidb::sql::SqlTypeId::Integer:
    return static_cast<std::int32_t>(value);
  case unidb::sql::SqlTypeId::BigInt:
    return value;
  default:
    throw std::invalid_argument("SQL type is not an integer type");
  }
}

std::int64_t integerValueAsInt64(const SqlIntegerValue &value) {
  return std::visit([](const auto integer) { return static_cast<std::int64_t>(integer); },
                    value);
}

} // namespace QueryExecutionService::domain
