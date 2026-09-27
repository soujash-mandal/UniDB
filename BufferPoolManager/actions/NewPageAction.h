#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"
#include "../port/WritePagePort.h"

class NewPageAction {
public:
  NewPageAction(BufferPool &bufferPool, AllocatePagePort &allocatePagePort,
                WritePagePort &writePagePort, EvictionPolicy &evictionPolicy);

  BufferPoolPageId execute();

private:
  BufferPool &bufferPool;
  AllocatePagePort &allocatePagePort;
  WritePagePort &writePagePort;
  EvictionPolicy &evictionPolicy;
};