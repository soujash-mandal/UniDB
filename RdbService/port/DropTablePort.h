#pragma once
#include <string>
namespace unidb::rdb {
class DropTablePort {
public:
  virtual ~DropTablePort() = default;
  virtual void drop(const std::string &tableName) = 0;
};
}