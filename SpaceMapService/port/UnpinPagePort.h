#pragma once

#include "../domain/SpaceMapPage.h"

class UnpinPagePort {
public:
  virtual ~UnpinPagePort() = default;

  virtual void unpinPage(SpaceMapPageId pageId) = 0;
};
