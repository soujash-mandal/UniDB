#pragma once

#include "../domain/CatalogTypes.h"

class CatalogFetchPagePort;
class CatalogWritePagePort;
class CatalogUnpinPagePort;

// Updates the tuple-root locator without exposing Catalog's stored Table type.
class UpdateCatalogTupleRootAction {
public:
  UpdateCatalogTupleRootAction(CatalogFetchPagePort &fetchPagePort,
                               CatalogWritePagePort &writePagePort,
                               CatalogUnpinPagePort &unpinPagePort);
  void execute(CatalogPageId rootPageId, CatalogTableId tableId,
               CatalogPageId tupleRootPageId);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
