#pragma once

#include "../../DiskManager/actions/ReadPageAction.h"
#include "../port/ReadFreePageMetadataPort.h"

class ReadFreePageMetadataAdapter : public ReadFreePageMetadataPort {
public:
  explicit ReadFreePageMetadataAdapter(ReadPageAction &readPageAction);

  void readPage(PageId pageId, Page &page) override;

private:
  ReadPageAction &readPageAction;
};
