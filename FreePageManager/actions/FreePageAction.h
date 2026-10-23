#pragma once

#include "../port/ReadFreePageMetadataPort.h"
#include "../port/WriteFreePageMetadataPort.h"

#include <cstdint>

class DiscardPagePort {
public:
  virtual ~DiscardPagePort() = default;
  virtual void discardPage(uint32_t pageId) = 0;
};

class FreePageAction {
public:
  FreePageAction(ReadFreePageMetadataPort &readMetadataPort,
                 WriteFreePageMetadataPort &writeMetadataPort,
                 DiscardPagePort &discardPagePort);
  void execute(uint32_t pageId);

private:
  ReadFreePageMetadataPort &readMetadataPort;
  WriteFreePageMetadataPort &writeMetadataPort;
  DiscardPagePort &discardPagePort;
};
