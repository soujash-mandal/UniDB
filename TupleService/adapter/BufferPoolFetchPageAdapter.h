#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class BufferPoolFetchPageAdapter : public FetchPagePort {
public:
  explicit BufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);

  Page &fetchPage(PageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
