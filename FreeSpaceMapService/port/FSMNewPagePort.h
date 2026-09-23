#pragma once

#include "../../core/NewPageResult.h"

class FSMNewPagePort {
public:
  virtual ~FSMNewPagePort() = default;
  virtual NewPageResult newPage() = 0;
};
