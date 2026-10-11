#pragma once

#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/WritePagePort.h"

class TupleBufferPoolWritePageAdapter : public WritePagePort {
public:
  explicit TupleBufferPoolWritePageAdapter(WritePageAction &writePageAction);
  void writePage(TuplePageId pageId, const TuplePage page) override;

private:
  WritePageAction &writePageAction;
};
