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
  CreateTableAction(CatalogFetchPagePort &fetchPagePort, CatalogNewPagePort &newPagePort,
                    CatalogWritePagePort &writePagePort, CatalogUnpinPagePort &unpinPagePort);

  CatalogTableId execute(CatalogPageId rootPageId, CatalogTableId tableId,
                         std::string name, std::vector<Column> columns);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogNewPagePort &newPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
