#pragma once

#include "../domain/RdbMetadataPage.h"
#include "../port/ReadRdbMetadataPagePort.h"
#include "../port/WriteRdbMetadataPagePort.h"

class AllocateNextTableIdAction {
public:
  AllocateNextTableIdAction(ReadRdbMetadataPagePort &readMetadataPagePort,
                            WriteRdbMetadataPagePort &writeMetadataPagePort);

  CatalogTableId execute(CatalogPageId pageId);

private:
  ReadRdbMetadataPagePort &readMetadataPagePort;
  WriteRdbMetadataPagePort &writeMetadataPagePort;
};
