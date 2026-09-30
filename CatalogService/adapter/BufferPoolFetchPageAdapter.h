#pragma once
#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class BufferPoolFetchPageAdapter : public FetchPagePort {
public:
  explicit BufferPoolFetchPageAdapter(FetchPageAction &fetchPageAction);
  CatalogPage fetchPage(CatalogPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
