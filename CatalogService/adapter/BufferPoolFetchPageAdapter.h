#pragma once
#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class CatalogBufferPoolFetchPageAdapter : public CatalogFetchPagePort {
public:
  explicit CatalogBufferPoolFetchPageAdapter(
      FetchPageAction &fetchPageAction);
  CatalogPage fetchPage(CatalogPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
