#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class FSMBufferPoolFetchPageAdapter : public FetchPagePort {
public:
  explicit FSMBufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);
  FSMPage fetchPage(FSMPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
