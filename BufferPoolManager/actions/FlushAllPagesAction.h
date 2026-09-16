#pragma once

#include "../domain/BufferPool.h"
#include "../port/WritePagePort.h"

class FlushAllPagesAction {

public:
  FlushAllPagesAction(BufferPool &bufferPool, WritePagePort &writePagePort);

  void execute();

private:
  BufferPool &bufferPool;
  WritePagePort &writePagePort;
};
