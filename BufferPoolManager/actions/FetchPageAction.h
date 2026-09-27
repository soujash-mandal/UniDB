#pragma once

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../domain/BufferPool.h"
#include "../port/ReadPagePort.h"
#include "../port/WritePagePort.h"

class FetchPageAction {

public:
  FetchPageAction(BufferPool &bufferPool, ReadPagePort &readPagePort,
                  WritePagePort &writePagePort, EvictionPolicy &evictionPolicy);

  BufferPoolPage execute(BufferPoolPageId pageId);

private:
  BufferPool &bufferPool;
  ReadPagePort &readPagePort;
  WritePagePort &writePagePort;
  EvictionPolicy &evictionPolicy;
};
