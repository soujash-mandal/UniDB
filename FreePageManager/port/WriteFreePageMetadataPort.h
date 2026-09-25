#pragma once

#include "../domain/FreePageMetadataPage.h"
#include "../domain/FreePageMetadataPageId.h"

class WriteFreePageMetadataPort {
public:
  virtual ~WriteFreePageMetadataPort() = default;
  virtual void writePage(FreePageMetadataPageId pageId,
                         FreePageMetadataPage page) = 0;
};
