#pragma once

#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/UnpinPagePort.h"

class TupleBufferPoolUnpinPageAdapter : public TupleUnpinPagePort {
public:
  explicit TupleBufferPoolUnpinPageAdapter(UnpinPageAction &unpinPageAction);
  void unpinPage(TuplePageId pageId) override;

private:
  UnpinPageAction &unpinPageAction;
};
