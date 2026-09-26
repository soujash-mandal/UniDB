#pragma once

#include "../port/ReadFreePageMetadataPort.h"
#include "../port/WriteFreePageMetadataPort.h"

#include <cstdint>

namespace {
using PageId = uint32_t;
}

class AllocatePageAction {
public:
  AllocatePageAction(ReadFreePageMetadataPort &readMetadataPort,
                     WriteFreePageMetadataPort &writeMetadataPort);

  PageId execute();

private:
  ReadFreePageMetadataPort &readMetadataPort;
  WriteFreePageMetadataPort &writeMetadataPort;
};
