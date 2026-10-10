#pragma once
#include "../../BufferPoolManager/actions/FetchPageAction.h"
#include "../port/FetchPagePort.h"

class CatalogBufferPoolFetchPageAdapter : public CatalogCatalogFetchPagePort {
public:
  explicit CatalogBufferPoolFetchPageAdapter(
      FetchPageAction &fetchPageAction);
  CatalogPage fetchPage(CatalogPageId pageId) override;

private:
  FetchPageAction &fetchPageAction;
};
