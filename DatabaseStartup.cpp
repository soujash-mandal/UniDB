#include "DatabaseStartup.h"

DatabaseStartup::DatabaseStartup(CreateFpmPageAction &createFpmPageAction,
                                 InitializeRdbAction &initializeRdbAction)
    : createFpmPageAction(createFpmPageAction),
      initializeRdbAction(initializeRdbAction) {}

void DatabaseStartup::execute(bool databaseAlreadyExists) {
  if (databaseAlreadyExists) {
    return;
  }

  // Page 0 is reserved for the Free Page Manager metadata.
  createFpmPageAction.execute(0);
  initializeRdbAction.execute();
}
