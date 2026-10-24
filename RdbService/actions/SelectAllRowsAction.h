#pragma once

#include "../domain/RdbTable.h"
#include "../port/CatalogTablePort.h"
#include "../port/RdbMetadataPort.h"
#include "../port/SpaceMapPort.h"
#include "../port/TuplePagePort.h"

#include <string>
#include <vector>

class SelectAllRowsAction {
public:
  SelectAllRowsAction(RdbMetadataPort &metadataPort,
                      CatalogTablePort &catalogTablePort,
                      SpaceMapPort &spaceMapPort,
                      TuplePagePort &tuplePagePort);

  std::vector<RdbRow> execute(const std::string &tableName);

private:
  RdbMetadataPort &metadataPort;
  CatalogTablePort &catalogTablePort;
  SpaceMapPort &spaceMapPort;
  TuplePagePort &tuplePagePort;
};
