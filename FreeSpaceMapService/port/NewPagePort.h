#pragma once

#include "../../core/NewPageResult.h"

class NewPagePort {
public:
  virtual ~NewPagePort() = default;
  virtual NewPageResult newPage() = 0;
};
