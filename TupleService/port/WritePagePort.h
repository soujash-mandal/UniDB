#pragma once

#include "../domain/TuplePage.h"

class TupleWritePagePort {
public:
  virtual ~TupleWritePagePort() = default;
  virtual void writePage(TuplePageId pageId, TuplePage page) = 0;
};
