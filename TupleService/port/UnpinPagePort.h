#pragma once

#include "../domain/TuplePage.h"

class UnpinPagePort {
public:
  virtual ~UnpinPagePort() = default;

  virtual void unpinPage(TuplePageId pageId) = 0;
};
