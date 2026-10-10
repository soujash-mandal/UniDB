#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class DropTableAction {
public:
  DropTableAction(CatalogFetchPagePort &fetchPagePort, CatalogWritePagePort &writePagePort,
                  CatalogUnpinPagePort &unpinPagePort);

  void execute(CatalogPageId rootPageId, CatalogTableId tableId);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
