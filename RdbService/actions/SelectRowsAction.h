#pragma once
#include "../port/SelectRowsPort.h"
namespace unidb::rdb {
class SelectRowsAction {
public:
  explicit SelectRowsAction(SelectRowsPort &port) : port(port) {}
  std::vector<std::vector<unidb::sql::SqlValue>>
  execute(const unidb::sql::SelectStatement &statement);

private:
  SelectRowsPort &port;
};
} // namespace unidb::rdb