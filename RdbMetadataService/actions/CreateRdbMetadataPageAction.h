#pragma once

#include "../port/NewPagePort.h"
#include "../port/WritePagePort.h"
#include "../port/UnpinPagePort.h"

class CreateRdbMetadataPageAction {
public:
  CreateRdbMetadataPageAction(NewPagePort &newPagePort,
                              WritePagePort &writePagePort,
                              UnpinPagePort &unpinPagePort);

  CatalogPageId execute();

private:
  NewPagePort &newPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
