#pragma once

#include "../domain/MetadataPage.h"

class WriteFreePageMetadataPort {
public:
  virtual ~WriteFreePageMetadataPort() = default;
  virtual void writePage(MetadataPageId pageId, MetadataPage page) = 0;
};
