#pragma once

#include "../domain/BufferPool.h"
#include "../port/WritePagePort.h"

class FlushPageAction {

public:
  FlushPageAction(BufferPool &bufferPool, WritePagePort &writePagePort);
  void execute(BufferPoolPageId pageId);

private:
  BufferPool &bufferPool;
  WritePagePort &writePagePort;
};
