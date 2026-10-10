#pragma once

#include "container/container.h"

class DatabaseStartup {
public:
  explicit DatabaseStartup(Container &container);

  // Returns true when a new database was initialized, false for an existing one.
  bool execute();

private:
  static constexpr char DATABASE_FILE[] = "database.db";
  static constexpr MetadataPageId RDB_METADATA_PAGE_ID = 1;

  void validateExistingDatabase();

  Container &container;
};
