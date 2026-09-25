#pragma once

#include "../../DiskManager/actions/WritePageAction.h"
#include "../port/WriteFreePageMetadataPort.h"

class WriteFreePageMetadataAdapter : public WriteFreePageMetadataPort {
public:
  explicit WriteFreePageMetadataAdapter(WritePageAction &writePageAction);

  void writePage(FreePageMetadataPageId pageId,
                 FreePageMetadataPage page) override;

private:
  WritePageAction &writePageAction;
};
