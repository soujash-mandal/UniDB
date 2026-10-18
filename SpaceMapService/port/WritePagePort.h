#pragma once

#include "../domain/SpaceMapPage.h"

class WritePagePort {
public:
  virtual ~WritePagePort() = default;

  virtual void writePage(SpaceMapPageId pageId, SpaceMapPage page) = 0;
};
