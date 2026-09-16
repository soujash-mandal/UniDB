#pragma once

#include "../../core/PageId.h"
#include "../port/ReadFreePageMetadataPort.h"
#include "../port/WriteFreePageMetadataPort.h"

class FreePageAction {
public:
  FreePageAction(ReadFreePageMetadataPort &readMetadataPort,
                 WriteFreePageMetadataPort &writeMetadataPort);

  void execute(PageId pageId);

private:
  ReadFreePageMetadataPort &readMetadataPort;
  WriteFreePageMetadataPort &writeMetadataPort;
};
