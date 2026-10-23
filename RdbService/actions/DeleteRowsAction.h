#pragma once
#include "../port/DeleteRowsPort.h"
namespace unidb::rdb {
class DeleteRowsAction {
public:
  explicit DeleteRowsAction(DeleteRowsPort &port) : port(port) {}
  std::size_t execute(const std::string &tableName,
                      const unidb::sql::WhereEquals &condition);

private:
  DeleteRowsPort &port;
};
} // namespace unidb::rdb