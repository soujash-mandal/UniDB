#pragma once

#include "../../core/PageId.h"
#include "../port/NewPagePort.h"

class CreatePageAction {
public:
  explicit CreatePageAction(NewPagePort &newPagePort);
  void execute(const PageId &pageId);

private:
  NewPagePort &newPagePort;
};
