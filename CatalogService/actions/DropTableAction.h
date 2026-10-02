#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class DropTableAction {
public:
  DropTableAction(FetchPagePort &fetchPagePort, WritePagePort &writePagePort,
                  UnpinPagePort &unpinPagePort);

  void execute(CatalogPageId rootPageId, CatalogTableId tableId);

private:
  FetchPagePort &fetchPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
