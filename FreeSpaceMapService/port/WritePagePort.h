#pragma once

#include "../domain/FSMPage.h"

class WritePagePort {
public:
  virtual ~WritePagePort() = default;

  virtual void writePage(FSMPageId pageId, FSMPage page) = 0;
};
