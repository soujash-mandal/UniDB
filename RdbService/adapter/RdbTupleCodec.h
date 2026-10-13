#pragma once

#include "../../SQLParserContract/sqlStatement.h"

#include <vector>

class RdbTupleCodec {
public:
  // The codec consumes the SQL contract's logical types, not CatalogService's
  // physical schema structs. Adapters translate catalog types at the boundary.
  static std::vector<char>
  encode(const std::vector<unidb::sql::SqlType> &schema,
         const std::vector<unidb::sql::SqlValue> &values);
  static std::vector<unidb::sql::SqlValue>
  decode(const std::vector<unidb::sql::SqlType> &schema,
         const std::vector<char> &bytes);
};
