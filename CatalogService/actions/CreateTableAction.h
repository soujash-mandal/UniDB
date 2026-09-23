#pragma once

#include <string>
#include <vector>

#include "../domain/Column.h"
#include "../domain/Table.h"

struct CreateTableRequest {
  std::string name;
  std::vector<Column> columns;
};

class CreateTableAction {
public:
  Table execute(const CreateTableRequest &request);
};
