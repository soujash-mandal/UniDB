#include "DatabaseStartup.h"

#include <filesystem>
#include <stdexcept>

bool DatabaseStartup::databaseExists(const std::string &databaseFile) {
  return std::filesystem::exists(databaseFile);
}

DatabaseStartup::DatabaseStartup(CreateFpmPageAction &createFpmPageAction,
                                 InitializeRdbAction &initializeRdbAction,
                                 FlushPageAction &flushPageAction)
    : createFpmPageAction(createFpmPageAction),
      initializeRdbAction(initializeRdbAction),
      flushPageAction(flushPageAction) {}

void DatabaseStartup::execute(bool databaseAlreadyExists) {
  if (databaseAlreadyExists) {
    return;
  }

  // Page 0 stores Free Page Manager metadata. Page 1 is the fixed RDB
  // metadata root, which stores the catalog root page ID.
  createFpmPageAction.execute(0);
  const InitializeRdbResult result = initializeRdbAction.execute();
  if (result.metadataPageId != RDB_METADATA_PAGE_ID) {
    throw std::runtime_error("RDB metadata page must be allocated at page 1");
  }

  // Persist both initialized pages before the process exits.
  flushPageAction.execute(result.metadataPageId);
  flushPageAction.execute(result.catalogRootPageId);
}
