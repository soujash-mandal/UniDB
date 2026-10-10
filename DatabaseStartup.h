#pragma once

#include "../FreePageManager/actions/CreateFpmPageAction.h"
#include "../RdbService/actions/InitializeRdbAction.h"

class DatabaseStartup {
public:
  DatabaseStartup(CreateFpmPageAction &createFpmPageAction,
                  InitializeRdbAction &initializeRdbAction);

  // Call only after checking whether database.db existed before opening it.
  // Existing databases must not be initialized again.
  void execute(bool databaseAlreadyExists);

private:
  CreateFpmPageAction &createFpmPageAction;
  InitializeRdbAction &initializeRdbAction;
};
