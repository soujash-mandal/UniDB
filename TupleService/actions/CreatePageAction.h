#pragma once

#include "../port/NewPagePort.h"

class CreatePageAction {
public:
  explicit CreatePageAction(NewPagePort &newPagePort);
  void execute();

private:
  NewPagePort &newPagePort;
};
