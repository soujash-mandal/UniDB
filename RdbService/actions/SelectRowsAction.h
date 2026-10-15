#pragma once
#include "../port/SelectRowsPort.h"
#include "../domain/RdbResult.h"
namespace unidb::rdb {
class SelectRowsAction {
public:
  explicit SelectRowsAction(SelectRowsPort &port) : port(port) {}
  RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>> execute(const unidb::sql::SelectStatement &statement);
private:
  SelectRowsPort &port;
};
}