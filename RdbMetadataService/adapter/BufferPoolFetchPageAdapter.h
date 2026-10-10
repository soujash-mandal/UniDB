#pragma once

#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class BufferPoolFetchPageAdapter : public RdbMetadataFetchPagePort {
public:
  explicit BufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);

  RdbMetadataPage fetchPage(CatalogPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
