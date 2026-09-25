#pragma once

#include "../domain/DiskPage.h"
#include "../domain/DiskPageId.h"

class DiskPort {
public:
  virtual ~DiskPort() = default;
  virtual DiskPage readPage(DiskPageId pageId) = 0;
  virtual void writePage(DiskPageId pageId, DiskPage &page) = 0;
};
