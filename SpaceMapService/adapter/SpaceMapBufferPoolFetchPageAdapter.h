#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class SpaceMapBufferPoolFetchPageAdapter : public FetchPagePort {
public:
  explicit SpaceMapBufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);
  SpaceMapPage fetchPage(SpaceMapPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
