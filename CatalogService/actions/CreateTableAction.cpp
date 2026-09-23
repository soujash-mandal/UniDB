#include "CreateTableAction.h"

#include <cstdint>
#include <stdexcept>

CreateTableAction::CreateTableAction() : nextTableId(1) {}

Table CreateTableAction::execute(const CreateTableRequest &request) {

  if (request.name.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  Table table;

  table.tableId = nextTableId++;
  table.name = request.name;
  table.columns = request.columns;
  table.firstFreeSpaceMapPageId = UINT32_MAX;

  return table;
}