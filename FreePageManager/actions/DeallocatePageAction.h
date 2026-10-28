#pragma once

#include "../port/ReadFreePageMetadataPort.h"
#include "../port/WriteFreePageMetadataPort.h"

#include <cstdint>

class DeallocatePageAction {
public:
  DeallocatePageAction(ReadFreePageMetadataPort &readMetadataPort,
                       WriteFreePageMetadataPort &writeMetadataPort);
  void execute(uint32_t pageId);

private:
  ReadFreePageMetadataPort &readMetadataPort;
  WriteFreePageMetadataPort &writeMetadataPort;
};
