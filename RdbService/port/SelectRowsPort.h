#pragma once
#include "../../SQLParserContract/sqlStatement.h"
#include <vector>
namespace unidb::rdb {
class SelectRowsPort {
public:
  virtual ~SelectRowsPort() = default;
  virtual std::vector<std::vector<unidb::sql::SqlValue>>
  select(const unidb::sql::SelectStatement &statement) = 0;
};
} // namespace unidb::rdb