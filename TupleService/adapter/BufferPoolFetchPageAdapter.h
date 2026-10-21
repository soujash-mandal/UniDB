#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../../BufferPoolManager/actions/UnpinPageAction.h"
#include "../port/FetchPagePort.h"

class TupleBufferPoolFetchPageAdapter : public TupleFetchPagePort {
public:
  TupleBufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction,
                                  UnpinPageAction &unpinPageAction);
  TuplePage fetchPage(TuplePageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
  UnpinPageAction &unpinPageAction;
};
