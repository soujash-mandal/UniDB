#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class TupleBufferPoolFetchPageAdapter : public TupleFetchPagePort {
public:
  explicit TupleBufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);
  TuplePage fetchPage(TuplePageId pageId) override;
private:
  FetchPageAction &fetchPageAction;
};
