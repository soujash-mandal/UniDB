#pragma once

#include "../port/NewPagePort.h"

class CreatePageAction {
public:
  explicit CreatePageAction(NewPagePort &newPagePort);
  TuplePageId execute();

private:
  NewPagePort &newPagePort;
};
