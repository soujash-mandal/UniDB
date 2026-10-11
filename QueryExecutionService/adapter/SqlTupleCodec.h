#pragma once

#include "../../SQLParserContract/sqlStatement.h"
#include "../../CatalogService/domain/CatalogPage.h"

#include <vector>

class SqlTupleCodec {
public:
  static std::vector<char> encode(const std::vector<Column> &schema,
                                  const std::vector<unidb::sql::SqlValue> &values);
  static std::vector<unidb::sql::SqlValue>
  decode(const std::vector<Column> &schema, const std::vector<char> &bytes);
};
