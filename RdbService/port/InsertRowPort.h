#pragma once
#include "../../SQLParserContract/sqlStatement.h"
namespace unidb::rdb {
class InsertRowPort {
public:
  virtual ~InsertRowPort() = default;
  virtual void insert(const unidb::sql::InsertStatement &statement) = 0;
};
} // namespace unidb::rdb