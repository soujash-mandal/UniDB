#pragma once

#include "container/container.h"

class DatabaseStartup {
public:
  explicit DatabaseStartup(Container &container);

  // Returns true if a new database was initialized.
  // Returns false if the database already existed.
  bool execute();

private:
  static constexpr MetadataPageId RDB_METADATA_PAGE_ID = 1;

  Container &container;
};
