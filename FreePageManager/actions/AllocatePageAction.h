#pragma once

#include "../../core/PageId.h"
#include "../port/ReadFreePageMetadataPort.h"
#include "../port/WriteFreePageMetadataPort.h"

class AllocatePageAction {
public:
  AllocatePageAction(ReadFreePageMetadataPort &readMetadataPort,
                     WriteFreePageMetadataPort &writeMetadataPort);

  PageId execute();

private:
  ReadFreePageMetadataPort &readMetadataPort;
  WriteFreePageMetadataPort &writeMetadataPort;
};
