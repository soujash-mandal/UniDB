#pragma once

#include "../domain/TuplePage.h"

class NewPagePort {
public:
  virtual ~NewPagePort() = default;

  virtual TuplePageId newPage() = 0;
};
