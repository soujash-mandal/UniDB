#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"

class UnpinPageAction {

public:
  UnpinPageAction(BufferPool &bufferPool, EvictionPolicy &evictionPolicy);

  void execute(BufferPoolPageId pageId, bool dirty);

private:
  BufferPool &bufferPool;
  EvictionPolicy &evictionPolicy;
};
