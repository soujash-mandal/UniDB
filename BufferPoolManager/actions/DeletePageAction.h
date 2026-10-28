#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"
#include "../port/DeallocatePagePort.h"

class DeletePageAction {
public:
  DeletePageAction(BufferPool &bufferPool, EvictionPolicy &evictionPolicy,
                   DeallocatePagePort &deallocatePagePort);
  void execute(BufferPoolPageId pageId);

private:
  BufferPool &bufferPool;
  EvictionPolicy &evictionPolicy;
  DeallocatePagePort &deallocatePagePort;
};
