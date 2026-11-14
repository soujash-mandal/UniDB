#pragma once

#include "../../type/type.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace QueryExecutionService::domain {

// Count UTF-8 Unicode code points. Reject malformed UTF-8 and NUL, neither
// of which can be stored in PostgreSQL character values.
std::size_t countSqlCharacters(std::string_view value);

// Compare character values using the declared SQL type. CHAR and unbounded
// BPCHAR ignore trailing ASCII spaces; VARCHAR and TEXT preserve them.
int compareSqlCharacterValues(const unidb::sql::SqlTypeDescriptor &type,
                              std::string_view left, std::string_view right);

// Apply assignment semantics for CHAR(n), VARCHAR(n), BPCHAR, and TEXT.
// CHAR(n) values are blank-padded. Unbounded BPCHAR, VARCHAR, and TEXT
// preserve their input bytes. Bounded types accept excess trailing spaces.
std::string
normalizeSqlCharacterValue(const unidb::sql::SqlTypeDescriptor &type,
                           std::string_view value);

} // namespace QueryExecutionService::domain
