#pragma once

#include "../domain/SpaceMapPage.h"

class SpaceMapWritePagePort {
public:
  virtual ~SpaceMapWritePagePort() = default;

  virtual void writePage(SpaceMapPageId pageId, SpaceMapPage page) = 0;
};
