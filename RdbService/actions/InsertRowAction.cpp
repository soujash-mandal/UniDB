#include "InsertRowAction.h"
namespace unidb::rdb {
void InsertRowAction::execute(const unidb::sql::InsertStatement &statement) {
  port.insert(statement);
}
}