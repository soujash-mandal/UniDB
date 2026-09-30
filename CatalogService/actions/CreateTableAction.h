#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <string>
#include <vector>

class CreateTableAction {
public:
  CreateTableAction(FetchPagePort &fetchPagePort, NewPagePort &newPagePort,
                    WritePagePort &writePagePort, UnpinPagePort &unpinPagePort);

  void execute(CatalogTableId tableId, std::string name,
               std::vector<Column> columns);

private:
  FetchPagePort &fetchPagePort;
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
