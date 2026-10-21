#pragma once

#include "../domain/TuplePage.h"

class TupleFetchPagePort {
public:
  virtual ~TupleFetchPagePort() = default;
  virtual TuplePage fetchPage(TuplePageId pageId) = 0;
};
