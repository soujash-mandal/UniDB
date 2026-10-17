#pragma once

#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/WritePagePort.h"

class TupleBufferPoolWritePageAdapter : public TupleWritePagePort {
public:
  explicit TupleBufferPoolWritePageAdapter(WritePageAction &writePageAction);
  void writePage(TuplePageId pageId, TuplePage page) override;

private:
  WritePageAction &writePageAction;
};
