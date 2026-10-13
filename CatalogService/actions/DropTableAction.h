#pragma once

#include "../domain/CatalogTypes.h"

class CatalogFetchPagePort;
class CatalogWritePagePort;
class CatalogUnpinPagePort;

class DropTableAction {
public:
  DropTableAction(CatalogFetchPagePort &fetchPagePort,
                  CatalogWritePagePort &writePagePort,
                  CatalogUnpinPagePort &unpinPagePort);

  void execute(CatalogPageId rootPageId, CatalogTableId tableId);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
