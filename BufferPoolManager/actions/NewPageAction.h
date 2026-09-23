#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../../core/NewPageResult.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"
#include "../port/WritePagePort.h"

class NewPageAction {
public:
  NewPageAction(BufferPool &bufferPool, AllocatePagePort &allocatePagePort,
                WritePagePort &writePagePort, EvictionPolicy &evictionPolicy);

  NewPageResult execute();

private:
  BufferPool &bufferPool;
  AllocatePagePort &allocatePagePort;
  WritePagePort &writePagePort;
  EvictionPolicy &evictionPolicy;
};