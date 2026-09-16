#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class ReadFreePageMetadataPort {
public:
  virtual ~ReadFreePageMetadataPort() = default;

  virtual void readPage(PageId pageId, Page &page) = 0;
};
