#pragma once

#include "../domain/RdbMetadataPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class AllocateNextTableIdAction {
public:
  AllocateNextTableIdAction(FetchPagePort &fetchPagePort,
                            WritePagePort &writePagePort,
                            UnpinPagePort &unpinPagePort);

  CatalogTableId execute(CatalogPageId pageId);

private:
  FetchPagePort &fetchPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
