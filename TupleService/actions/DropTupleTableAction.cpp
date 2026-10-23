#include "DropTupleTableAction.h"

#include "../domain/TuplePage.h"

#include <limits>
#include <vector>

DropTupleTableAction::DropTupleTableAction(TupleFetchPagePort &fetchPagePort,
                                           TupleUnpinPagePort &unpinPagePort,
                                           FreeTuplePagePort &freeTuplePagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      freeTuplePagePort(freeTuplePagePort) {}

void DropTupleTableAction::execute(uint32_t firstPageId) {
  if (firstPageId == std::numeric_limits<uint32_t>::max()) {
    return;
  }

  std::vector<uint32_t> pageIds;
  uint32_t pageId = firstPageId;
  while (pageId != std::numeric_limits<uint32_t>::max()) {
    auto page = fetchPagePort.fetchPage(pageId);
    const uint32_t nextPageId = page.getNextPageId();
    unpinPagePort.unpinPage(pageId);
    pageIds.push_back(pageId);
    pageId = nextPageId;
  }

  for (const uint32_t ownedPageId : pageIds) {
    freeTuplePagePort.freePage(ownedPageId);
  }
}
