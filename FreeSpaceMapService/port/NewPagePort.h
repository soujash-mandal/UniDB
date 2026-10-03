#pragma once

#include "../domain/FSMPage.h"

class NewPagePort {
public:
  virtual ~NewPagePort() = default;

  virtual FSMPageId newPage() = 0;
};
