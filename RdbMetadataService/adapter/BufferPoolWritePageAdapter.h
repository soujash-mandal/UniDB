#pragma once

#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/WritePagePort.h"

class BufferPoolWritePageAdapter : public RdbMetadataWritePagePort {
public:
  explicit BufferPoolWritePageAdapter(WritePageAction &writePageAction);

  void writePage(CatalogPageId pageId, RdbMetadataPage page) override;

private:
  WritePageAction &writePageAction;
};
