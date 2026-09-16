#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class WritePagePort {
public:
  virtual ~WritePagePort() = default;

  virtual void writePage(PageId pageId, const Page &page) = 0;
};
