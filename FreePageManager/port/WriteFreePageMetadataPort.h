#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class WriteFreePageMetadataPort {
public:
  virtual ~WriteFreePageMetadataPort() = default;

  virtual void writePage(PageId pageId, const Page &page) = 0;
};
