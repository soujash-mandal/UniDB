#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

class PagePort {
public:
  virtual ~PagePort() = default;

  virtual void readPage(const PageId &pageId, Page &page) = 0;
  virtual void writePage(const PageId &pageId, const Page &page) = 0;
};
