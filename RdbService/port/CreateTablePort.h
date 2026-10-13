#pragma once
#include "../../SQLParserContract/sqlStatement.h"
#include <cstdint>
namespace unidb::rdb {
class CreateTablePort {
public:
  virtual ~CreateTablePort() = default;
  virtual uint32_t create(const unidb::sql::CreateTableStatement &statement) = 0;
};
}