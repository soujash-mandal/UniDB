#pragma once

#include "../../core/PageId.h"
#include "../port/ReadFreePageMetadataPort.h"

class AllocatePageAction {
public:
  explicit AllocatePageAction(ReadFreePageMetadataPort &readMetadataPort);

  PageId execute();

private:
  ReadFreePageMetadataPort &readMetadataPort;
};
