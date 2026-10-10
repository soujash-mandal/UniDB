#pragma once

#include "../domain/type.h"
#include "../port/CreateFpmPagePort.h"
#include "../port/InitializeRdbPort.h"

class DatabaseStartup {
public:
  DatabaseStartup(CreateFpmPagePort &createFpmPagePort,
                  InitializeRdbPort &initializeRdbPort);

  RdbMetadataPageId execute();

private:
  CreateFpmPagePort &createFpmPagePort;
  InitializeRdbPort &initializeRdbPort;
};
