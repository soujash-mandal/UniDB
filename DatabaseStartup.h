#pragma once

#include "container/container.h"

class DatabaseStartup {
public:
  // Returns true if a new database was initialized.
  // Returns false if the database already existed.
  static bool execute(Container &container);

private:
  static constexpr MetadataPageId RDB_METADATA_PAGE_ID = 1;
};
