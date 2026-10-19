#pragma once

#include "../domain/SpaceMapPage.h"

class FetchPagePort {
public:
  virtual ~FetchPagePort() = default;

  virtual SpaceMapPage fetchPage(SpaceMapPageId pageId) = 0;
};
