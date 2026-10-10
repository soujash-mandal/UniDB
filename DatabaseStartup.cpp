#include "DatabaseStartup.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

DatabaseStartup::DatabaseStartup(Container &container) : container(container) {}

bool DatabaseStartup::execute() {
  const bool databaseAlreadyExists = std::filesystem::exists(DATABASE_FILE);

  if (!databaseAlreadyExists) {
    std::ofstream databaseFile(DATABASE_FILE, std::ios::binary);
    if (!databaseFile.is_open()) {
      throw std::runtime_error("Could not create database file");
    }
    databaseFile.close();
    if (!databaseFile) {
      throw std::runtime_error("Could not close newly created database file");
    }

    // Page 0 stores Free Page Manager metadata. The RDB metadata root must be
    // allocated at page 1; it stores the catalog root page ID.
    container.createFpmPageAction().execute(0);
    const MetadataPageId metadataPageId =
        container.initializeRdbAction().execute();
    if (metadataPageId != RDB_METADATA_PAGE_ID) {
      throw std::runtime_error(
          "RDB metadata page must be allocated at page 1");
    }

    container.flushPageAction().execute(metadataPageId);
    const CatalogPageId catalogRootPageId =
        container.getCatalogRootPageIdAction().execute(metadataPageId);
    container.flushPageAction().execute(catalogRootPageId);
    return true;
  }

  validateExistingDatabase();
  return false;
}

void DatabaseStartup::validateExistingDatabase() {
  // Validate that the FPM root page can be read.
  container.fetchPageAction().execute(0);
  container.unpinPageAction().execute(0);

  const CatalogPageId catalogRootPageId =
      container.getCatalogRootPageIdAction().execute(RDB_METADATA_PAGE_ID);
  if (catalogRootPageId == RdbMetadataPage::INVALID_PAGE_ID ||
      catalogRootPageId == 0 ||
      catalogRootPageId == RDB_METADATA_PAGE_ID) {
    throw std::runtime_error("Database has an invalid catalog root page ID");
  }

  // A successful read confirms that the referenced catalog page exists.
  container.fetchPageAction().execute(catalogRootPageId);
  container.unpinPageAction().execute(catalogRootPageId);
}
