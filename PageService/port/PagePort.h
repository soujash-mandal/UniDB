#pragma once

#include "../../core/Page.h"

class PagePort {
public:
  virtual ~PagePort() = default;

  virtual void readPage(PageId pageId, Page &page) = 0;
  virtual void writePage(PageId pageId, const Page &page) = 0;
};
