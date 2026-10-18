#pragma once

#include "../domain/FSMPage.h"

class UnpinPagePort {
public:
  virtual ~UnpinPagePort() = default;

  virtual void unpinPage(FSMPageId pageId) = 0;
};
