#pragma once
#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/CatalogWritePagePort.h"

class CatalogBufferPoolWritePageAdapter : public CatalogWritePagePort {
public:
  explicit CatalogBufferPoolWritePageAdapter(WritePageAction &writePageAction);
  void writePage(CatalogPageId pageId, CatalogPage page) override;

private:
  WritePageAction &writePageAction;
};
