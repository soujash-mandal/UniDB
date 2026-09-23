#pragma once

#include <string>
#include <vector>

#include "../../core/TableId.h"
#include "../domain/Column.h"
#include "../domain/Table.h"

struct CreateTableRequest {
  std::string name;
  std::vector<Column> columns;
};

class CreateTableAction {
public:
  CreateTableAction();

  Table execute(const CreateTableRequest &request);

private:
  TableId nextTableId;
};
