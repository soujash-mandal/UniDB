#pragma once

#include "../../type/type.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace QueryExecutionService::domain {

// Count UTF-8 Unicode code points. Reject malformed UTF-8 rather than
// confusing byte length with SQL character length.
std::size_t countSqlCharacters(std::string_view value);

// Compare character values using the declared SQL type. CHAR ignores trailing
// ASCII spaces; VARCHAR and TEXT preserve them as significant.
int compareSqlCharacterValues(const unidb::sql::SqlTypeDescriptor &type,
                              std::string_view left, std::string_view right);

// Apply assignment semantics for CHAR(n), VARCHAR(n), and TEXT.
// CHAR values are blank-padded. VARCHAR/TEXT preserve their input bytes.
// PostgreSQL's excess-trailing-space exception is applied to bounded types.
std::string
normalizeSqlCharacterValue(const unidb::sql::SqlTypeDescriptor &type,
                           std::string_view value);

} // namespace QueryExecutionService::domain
