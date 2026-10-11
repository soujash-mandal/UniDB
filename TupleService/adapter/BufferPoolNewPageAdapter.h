#pragma once

#include "../../BufferPoolManager/actions/NewPageAction.h"
#include "../port/NewPagePort.h"

class TupleBufferPoolNewPageAdapter : public TupleNewPagePort {
public:
  explicit TupleBufferPoolNewPageAdapter(NewPageAction &newPageAction);
  TuplePageId newPage() override;
private:
  NewPageAction &newPageAction;
};
