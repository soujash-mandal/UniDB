#pragma once

#include "../../DiskManager/actions/ReadPageAction.h"
#include "../port/ReadFreePageMetadataPort.h"

class ReadFreePageMetadataAdapter : public ReadFreePageMetadataPort {
public:
  explicit ReadFreePageMetadataAdapter(ReadPageAction &readPageAction);
  MetadataPage readPage(MetadataPageId pageId) override;

private:
  ReadPageAction &readPageAction;
};
