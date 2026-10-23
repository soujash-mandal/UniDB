#pragma once
#include "../../SQLParserContract/sqlStatement.h"
#include <cstddef>
#include <string>
namespace unidb::rdb {
class DeleteRowsPort {
public:
  virtual ~DeleteRowsPort() = default;
  virtual std::size_t erase(const std::string &tableName,
                            const unidb::sql::WhereEquals &condition) = 0;
};
} // namespace unidb::rdb