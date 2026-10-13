#pragma once
#include "../port/DropTablePort.h"
namespace unidb::rdb {
class DropTableAction {
public:
  explicit DropTableAction(DropTablePort &port) : port(port) {}
  void execute(const std::string &tableName);
private:
  DropTablePort &port;
};
}