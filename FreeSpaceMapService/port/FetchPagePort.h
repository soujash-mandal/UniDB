#pragma once

#include "../domain/FSMPage.h"

class FetchPagePort {
public:
  virtual ~FetchPagePort() = default;

  virtual FSMPage fetchPage(FSMPageId pageId) = 0;
};
