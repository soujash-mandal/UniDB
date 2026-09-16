#pragma once

#include "../../core/PageId.h"
#include "../domain/BufferPool.h"

class UnpinPageAction {

public:
  explicit UnpinPageAction(BufferPool &bufferPool);
  void execute(PageId pageId, bool dirty);

private:
  BufferPool &bufferPool;
};
