#pragma once

#include "../../type/type.h"

#include <vector>

namespace QueryExecutionService::domain {

// Versioned, self-contained encoding for one exact NUMERIC value.
class SqlNumericRowCodec {
public:
  static std::vector<char>
  encode(const unidb::sql::SqlNumericValue &value);

  static unidb::sql::SqlNumericValue
  decode(const std::vector<char> &bytes);
};

} // namespace QueryExecutionService::domain
