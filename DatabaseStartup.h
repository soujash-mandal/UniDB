#pragma once

#include "FreePageManager/actions/CreateFpmPageAction.h"
#include "RdbService/actions/InitializeRdbAction.h"

#include <string>

class DatabaseStartup {
public:
  DatabaseStartup(CreateFpmPageAction &createFpmPageAction,
                  InitializeRdbAction &initializeRdbAction);

  // Check before FileDiskManagerAdapter is constructed because it creates a
  // missing database file.
  static bool databaseExists(const std::string &databaseFile);

  void execute(bool databaseAlreadyExists);

private:
  static constexpr MetadataPageId RDB_METADATA_PAGE_ID = 1;

  CreateFpmPageAction &createFpmPageAction;
  InitializeRdbAction &initializeRdbAction;
};
