#pragma once

#include "../../core/Page.h"

class NewPagePort {
public:
  virtual ~NewPagePort() = default;

  virtual Page &newPage() = 0;
};
