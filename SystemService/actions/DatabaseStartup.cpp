#include "DatabaseStartup.h"

DatabaseStartup::DatabaseStartup(
    CreateFpmPagePort &createFpmPagePort,
    InitializeRdbPort &initializeRdbPort)
    : createFpmPagePort(createFpmPagePort),
      initializeRdbPort(initializeRdbPort) {}

RdbMetadataPageId DatabaseStartup::execute() {
  createFpmPagePort.initialize();
  return initializeRdbPort.initialize();
}
