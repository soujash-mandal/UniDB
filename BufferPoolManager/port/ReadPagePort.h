#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class ReadPagePort {
public:
  virtual ~ReadPagePort() = default;

  virtual void readPage(PageId pageId, Page &page) = 0;
};
