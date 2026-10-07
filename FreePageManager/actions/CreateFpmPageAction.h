#pragma once

#include "../domain/type.h"
#include "../port/WriteFreePageMetadataPort.h"

class CreateFpmPageAction {
public:
  explicit CreateFpmPageAction(WriteFreePageMetadataPort &writeMetadataPort);
  void execute(FpmPageId pageId);

private:
  WriteFreePageMetadataPort &writeMetadataPort;
};
