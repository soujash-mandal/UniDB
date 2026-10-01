#pragma once

#include "../../BufferPoolManager/actions/WritePageAction.h"
#include "../port/WriteRdbMetadataPagePort.h"

class BufferPoolManagerWriteRdbMetadataPageAdapter
    : public WriteRdbMetadataPagePort {
public:
  explicit BufferPoolManagerWriteRdbMetadataPageAdapter(
      WritePageAction &writePageAction);

  void writePage(CatalogPageId pageId, RdbMetadataPage page) override;

private:
  WritePageAction &writePageAction;
};
