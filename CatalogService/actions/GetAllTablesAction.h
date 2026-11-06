#pragma once
#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include <vector>
class GetAllTablesAction {
public:
  GetAllTablesAction(CatalogFetchPagePort &, CatalogUnpinPagePort &);
  std::vector<Table> execute(CatalogPageId rootPageId);
private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
