#pragma once

#include "../domain/BufferPoolPage.h"
#include "../domain/BufferPoolPageId.h"

class ReadPagePort {
public:
  virtual ~ReadPagePort() = default;
  virtual BufferPoolPage readDiskPage(BufferPoolPageId pageId) = 0;
};
