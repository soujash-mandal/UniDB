#pragma once
#include "../port/InsertRowPort.h"
#include "../domain/RdbResult.h"
namespace unidb::rdb {
class InsertRowAction {
public:
  explicit InsertRowAction(InsertRowPort &port) : port(port) {}
  RdbResult<std::size_t> execute(const unidb::sql::InsertStatement &statement);
private:
  InsertRowPort &port;
};
}