#pragma once
#include "../port/CreateTablePort.h"
namespace unidb::rdb {
class CreateTableAction {
public:
  explicit CreateTableAction(CreateTablePort &port) : port(port) {}
  uint32_t execute(const unidb::sql::CreateTableStatement &statement);
private:
  CreateTablePort &port;
};
}