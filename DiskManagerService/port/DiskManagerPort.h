#pragma once
#include "../domain/Page.h"

class DiskManagerPort {
public:
  virtual ~DiskManagerPort() = default;
  virtual void writePage(PageId pageId, const Page &page) = 0;
  virtual void readPage(PageId pageId, Page &page) = 0;
};
