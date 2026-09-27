#pragma once

#include "../port/WriteFreePageMetadataPort.h"

class InitializeMetadataPageAction {
public:
  explicit InitializeMetadataPageAction(
      WriteFreePageMetadataPort &writeMetadataPort);

  void execute();

private:
  WriteFreePageMetadataPort &writeMetadataPort;
};
