#pragma once
#include "../port/CreateTablePort.h"
#include "../domain/RdbResult.h"
namespace unidb::rdb {
class CreateTableAction {
public:
  explicit CreateTableAction(CreateTablePort &port) : port(port) {}
  RdbResult<uint32_t> execute(const unidb::sql::CreateTableStatement &statement);
private:
  CreateTablePort &port;
};
}