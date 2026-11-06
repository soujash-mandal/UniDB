#include "GetAllTablesAction.h"
#include <iterator>
#include <stdexcept>
#include <unordered_set>
GetAllTablesAction::GetAllTablesAction(CatalogFetchPagePort &fetch,
                                       CatalogUnpinPagePort &unpin)
    : fetchPagePort(fetch), unpinPagePort(unpin) {}
std::vector<Table> GetAllTablesAction::execute(CatalogPageId rootPageId) {
  std::vector<Table> tables;
  std::unordered_set<CatalogPageId> visited;
  auto current = rootPageId;
  while (current != CATALOG_INVALID_PAGE_ID) {
    if (!visited.insert(current).second)
      throw std::runtime_error("Catalog page chain is invalid");
    CatalogPage page = fetchPagePort.fetchPage(current);
    CatalogPageId next;
    std::vector<Table> pageTables;
    try {
      next = page.getNextPageId();
      pageTables = page.getTables();
    } catch (...) {
      try { unpinPagePort.unpinPage(current); } catch (...) {}
      throw;
    }
    unpinPagePort.unpinPage(current);
    tables.insert(tables.end(), std::make_move_iterator(pageTables.begin()),
                  std::make_move_iterator(pageTables.end()));
    current = next;
  }
  return tables;
}
