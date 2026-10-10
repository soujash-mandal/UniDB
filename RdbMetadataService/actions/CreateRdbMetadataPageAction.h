#pragma once

#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class CreateRdbMetadataPageAction {
public:
  CreateRdbMetadataPageAction(RdbMetadataNewPagePort &newPagePort,
                              RdbMetadataWritePagePort &writePagePort,
                              RdbMetadataUnpinPagePort &unpinPagePort);

  CatalogPageId execute();

private:
  RdbMetadataNewPagePort &newPagePort;
  RdbMetadataWritePagePort &writePagePort;
  RdbMetadataUnpinPagePort &unpinPagePort;
};
