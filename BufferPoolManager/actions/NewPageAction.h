#pragma once

#include "../../core/Page.h"
#include "../domain/BufferPool.h"
#include "../port/AllocatePagePort.h"

class NewPageAction {
public:
  explicit NewPageAction(BufferPool &bufferPool,
                         AllocatePagePort &allocatePagePort);
  Page &execute();

private:
  BufferPool &bufferPool;
  AllocatePagePort &allocatePagePort;
};
