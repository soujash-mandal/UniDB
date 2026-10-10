#include "DatabaseStartup.h"

#include <filesystem>
#include <stdexcept>

bool DatabaseStartup::databaseExists(const std::string &databaseFile) {
  return std::filesystem::exists(databaseFile);
}

DatabaseStartup::DatabaseStartup(CreateFpmPageAction &createFpmPageAction,
                                 InitializeRdbAction &initializeRdbAction)
    : createFpmPageAction(createFpmPageAction),
      initializeRdbAction(initializeRdbAction) {}

void DatabaseStartup::execute(bool databaseAlreadyExists) {
  if (databaseAlreadyExists) {
    return;
  }

  // Page 0 stores Free Page Manager metadata. Page 1 is the fixed RDB
  // metadata root, which stores the catalog root page ID.
  createFpmPageAction.execute(0);
  const MetadataPageId metadataPageId = initializeRdbAction.execute();
  if (metadataPageId != RDB_METADATA_PAGE_ID) {
    throw std::runtime_error("RDB metadata page must be allocated at page 1");
  }
}
