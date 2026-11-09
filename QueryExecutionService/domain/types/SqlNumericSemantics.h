#pragma once

#include "../../type/type.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace QueryExecutionService::domain {

// Parse a SQL decimal literal, including an optional sign and exponent, without
// converting through a binary floating-point type.
unidb::sql::SqlNumericValue parseSqlNumericLiteral(std::string_view literal);

// Build the canonical finite-decimal representation from an unsigned digit
// sequence, sign, and decimal scale. The coefficient must contain digits only.
unidb::sql::SqlNumericValue normalizeSqlNumericValue(bool negative,
                                                     std::string coefficient,
                                                     std::int32_t scale);

// Format a canonical finite-decimal value without losing precision.
std::string formatSqlNumericValue(const unidb::sql::SqlNumericValue &value);

// Compare canonical finite-decimal values exactly. Returns -1, 0, or 1.
int compareSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                            const unidb::sql::SqlNumericValue &right);

// Exact finite-decimal arithmetic. Inputs must be canonical values.
unidb::sql::SqlNumericValue
addSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                    const unidb::sql::SqlNumericValue &right);
unidb::sql::SqlNumericValue
subtractSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                         const unidb::sql::SqlNumericValue &right);
unidb::sql::SqlNumericValue
multiplySqlNumericValues(const unidb::sql::SqlNumericValue &left,
                         const unidb::sql::SqlNumericValue &right);

// Divide exactly at the requested result scale, rounding half away from zero.
// This is a low-level decimal primitive; SQL type coercion chooses the scale.
unidb::sql::SqlNumericValue
divideSqlNumericValues(const unidb::sql::SqlNumericValue &left,
                       const unidb::sql::SqlNumericValue &right,
                       std::int32_t resultScale);

} // namespace QueryExecutionService::domain
