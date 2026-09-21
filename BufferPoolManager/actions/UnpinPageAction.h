#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../../core/PageId.h"
#include "../domain/BufferPool.h"

class UnpinPageAction {

public:
  UnpinPageAction(BufferPool &bufferPool, EvictionPolicy &evictionPolicy);

  void execute(PageId pageId, bool dirty);

private:
  BufferPool &bufferPool;
  EvictionPolicy &evictionPolicy;
};
