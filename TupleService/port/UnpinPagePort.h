#pragma once

#include "../domain/TuplePage.h"

class TupleUnpinPagePort {
public:
  virtual ~TupleUnpinPagePort() = default;
  virtual void unpinPage(TuplePageId pageId) = 0;
};
