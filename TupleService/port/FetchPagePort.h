#pragma once

#include "../domain/TuplePage.h"

class FetchPagePort {
public:
  virtual ~FetchPagePort() = default;

  virtual TuplePage fetchPage(TuplePageId pageId) = 0;
};
