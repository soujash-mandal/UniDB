#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class TupleBufferPoolFetchPageAdapter : public FetchPagePort {
public:
  explicit TupleBufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);
  TuplePage fetchPage(TuplePageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
