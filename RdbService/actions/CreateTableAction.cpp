#include "CreateTableAction.h"
namespace unidb::rdb {
uint32_t
CreateTableAction::execute(const unidb::sql::CreateTableStatement &statement) {
  return port.create(statement);
}
} // namespace unidb::rdb