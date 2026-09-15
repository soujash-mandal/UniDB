#pragma once
#include "../../core/PageId.h"
#include "../port/PagePort.h"

class CreatePageAction {
public:
  explicit CreatePageAction(PagePort &pagePort);

  void execute(const PageId &pageId);

private:
  PagePort &pagePort;
};
