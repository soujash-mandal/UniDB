#pragma once

#include "../port/DiskWritePagePort.h"

class InitializeCatalogAction {
public:
  explicit InitializeCatalogAction(DiskWritePagePort &diskWritePagePort);

  void execute();

private:
  DiskWritePagePort &diskWritePagePort;
};
