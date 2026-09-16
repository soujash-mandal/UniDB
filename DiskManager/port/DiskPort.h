#pragma once
#include "../../core/Page.h"
#include "../../core/PageId.h"

class DiskPort {
public:
  virtual ~DiskPort() = default;
  virtual void writePage(const PageId &pageId, const Page &page) = 0;
  virtual void readPage(const PageId &pageId, Page &page) = 0;
};
