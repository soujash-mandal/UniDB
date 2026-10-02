#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateRootCatalogPageAction {
public:
  CreateRootCatalogPageAction(NewPagePort &newPagePort,
                              WritePagePort &writePagePort,
                              UnpinPagePort &unpinPagePort);
  CatalogPageId execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
