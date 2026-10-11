#pragma once

#include "../domain/TuplePage.h"

class TupleNewPagePort {
public:
  virtual ~TupleNewPagePort() = default;
  virtual TuplePageId newPage() = 0;
};
