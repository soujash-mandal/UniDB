#pragma once
#include "../port/InsertRowPort.h"
namespace unidb::rdb {
class InsertRowAction {
public:
  explicit InsertRowAction(InsertRowPort &port) : port(port) {}
  void execute(const unidb::sql::InsertStatement &statement);

private:
  InsertRowPort &port;
};
} // namespace unidb::rdb