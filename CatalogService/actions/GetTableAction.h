#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class GetTableAction {
public:
  GetTableAction(FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort);

  Table execute(CatalogPageId rootPageId, CatalogTableId tableId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
