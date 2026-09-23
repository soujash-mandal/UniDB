#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FSMFetchPagePort.h"

class FSMBufferPoolFetchPageAdapter : public FSMFetchPagePort {
public:
  explicit FSMBufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);

  Page &fetchPage(PageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
