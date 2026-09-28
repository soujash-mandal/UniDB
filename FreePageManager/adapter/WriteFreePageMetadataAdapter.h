#pragma once

#include "../../DiskManager/actions/WriteDiskPageAction.h"
#include "../port/WriteFreePageMetadataPort.h"

class WriteFreePageMetadataAdapter : public WriteFreePageMetadataPort {
public:
  explicit WriteFreePageMetadataAdapter(WriteDiskPageAction &writePageAction);
  void writePage(MetadataPageId pageId, MetadataPage page) override;

private:
  WriteDiskPageAction &writePageAction;
};
