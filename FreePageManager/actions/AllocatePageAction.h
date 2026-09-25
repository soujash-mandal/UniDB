#pragma once

#include "../port/ReadFreePageMetadataPort.h"
#include "../port/WriteFreePageMetadataPort.h"

class AllocatePageAction {
public:
  AllocatePageAction(ReadFreePageMetadataPort &readMetadataPort,
                     WriteFreePageMetadataPort &writeMetadataPort);

  FreePageMetadataPageId execute();

private:
  ReadFreePageMetadataPort &readMetadataPort;
  WriteFreePageMetadataPort &writeMetadataPort;
};
