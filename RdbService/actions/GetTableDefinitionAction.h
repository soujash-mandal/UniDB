#pragma once

#include "../domain/RdbTable.h"
#include "../port/CatalogTablePort.h"
#include "../port/RdbMetadataPort.h"

#include <string>

class GetTableDefinitionAction {
public:
  GetTableDefinitionAction(RdbMetadataPort &metadataPort,
                           CatalogTablePort &catalogTablePort);

  RdbTableDefinition execute(const std::string &tableName);

private:
  RdbMetadataPort &metadataPort;
  CatalogTablePort &catalogTablePort;
};
