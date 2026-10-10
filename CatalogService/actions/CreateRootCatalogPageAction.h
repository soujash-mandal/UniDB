#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateRootCatalogPageAction {
public:
  CreateRootCatalogPageAction(CatalogNewPagePort &newPagePort,
                              CatalogWritePagePort &writePagePort,
                              CatalogUnpinPagePort &unpinPagePort);
  CatalogPageId execute();

private:
  CatalogNewPagePort &newPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
