#pragma once

#include "../domain/CatalogTypes.h"

#include <string>
#include <vector>

class CatalogFetchPagePort;
class CatalogNewPagePort;
class CatalogWritePagePort;
class CatalogUnpinPagePort;

class CreateTableAction {
public:
  CreateTableAction(CatalogFetchPagePort &fetchPagePort,
                    CatalogNewPagePort &newPagePort,
                    CatalogWritePagePort &writePagePort,
                    CatalogUnpinPagePort &unpinPagePort);

  CatalogTableId execute(CatalogPageId rootPageId, CatalogTableId tableId,
                         std::string name, std::vector<Column> columns);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogNewPagePort &newPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
