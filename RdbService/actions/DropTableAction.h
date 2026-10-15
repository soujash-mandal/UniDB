#pragma once
#include "../port/DropTablePort.h"
#include "../domain/RdbResult.h"
namespace unidb::rdb {
class DropTableAction {
public:
  explicit DropTableAction(DropTablePort &port) : port(port) {}
  RdbStatus execute(const std::string &tableName);
private:
  DropTablePort &port;
};
}