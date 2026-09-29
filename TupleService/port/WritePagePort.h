#pragma once

#include "../domain/TuplePage.h"

class WritePagePort {
public:
  virtual ~WritePagePort() = default;

  virtual void writePage(TuplePageId pageId, TuplePage page) = 0;
};
