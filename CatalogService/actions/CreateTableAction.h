#pragma once

#include "../domain/CatalogPage.h"
#include "../port/AllocateTableIdPort.h"
#include "../port/FetchPagePort.h"
#include "../port/GetCatalogRootPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

#include <string>
#include <vector>

class CreateTableAction {
public:
  CreateTableAction(FetchPagePort &fetchPagePort, NewPagePort &newPagePort,
                    WritePagePort &writePagePort, UnpinPagePort &unpinPagePort,
                    GetCatalogRootPagePort &getCatalogRootPagePort,
                    AllocateTableIdPort &allocateTableIdPort);

  CatalogTableId execute(std::string name, std::vector<Column> columns);

private:
  FetchPagePort &fetchPagePort;
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
  GetCatalogRootPagePort &getCatalogRootPagePort;
  AllocateTableIdPort &allocateTableIdPort;
};
