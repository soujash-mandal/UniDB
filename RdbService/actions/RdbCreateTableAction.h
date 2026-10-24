#pragma once

#include "../domain/RdbTable.h"
#include "../port/CatalogTablePort.h"
#include "../port/RdbMetadataPort.h"

#include <string>
#include <vector>

class RdbCreateTableAction {
public:
  RdbCreateTableAction(RdbMetadataPort &metadataPort,
                       CatalogTablePort &catalogTablePort);

  uint32_t execute(const std::string &tableName,
                   const std::vector<RdbColumn> &columns);

private:
  RdbMetadataPort &metadataPort;
  CatalogTablePort &catalogTablePort;
};
