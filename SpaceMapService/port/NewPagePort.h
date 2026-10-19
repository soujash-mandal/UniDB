#pragma once

#include "../domain/SpaceMapPage.h"

class NewPagePort {
public:
  virtual ~NewPagePort() = default;

  virtual SpaceMapPageId newPage() = 0;
};
