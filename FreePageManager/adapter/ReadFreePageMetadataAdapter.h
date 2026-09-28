#pragma once

#include "../../DiskManager/actions/ReadDiskPageAction.h"
#include "../port/ReadFreePageMetadataPort.h"

class ReadFreePageMetadataAdapter : public ReadFreePageMetadataPort {
public:
  explicit ReadFreePageMetadataAdapter(ReadDiskPageAction &readDiskPageAction);
  MetadataPage readDiskPage(MetadataPageId pageId) override;

private:
  ReadDiskPageAction &readDiskPageAction;
};
