#include "FindPageWithSpaceAction.h"

#include <stdexcept>

FindPageWithSpaceAction::FindPageWithSpaceAction(
    FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort,
    SpaceMapWritePagePort &writePagePort, NewPagePort &newPagePort,
    CreateTuplePagePort &createTuplePagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      writePagePort(writePagePort), newPagePort(newPagePort),
      createTuplePagePort(createTuplePagePort) {}

FindPageWithSpaceResult
FindPageWithSpaceAction::execute(SpaceMapPageId rootSpaceMapPageId,
                                 uint32_t requiredSpace) {
  SpaceMapPageId spaceMapPageId = rootSpaceMapPageId;
  while (SpaceMapPage::isValidPage(spaceMapPageId)) {
    SpaceMapPage spaceMapPage = fetchPagePort.fetchPage(spaceMapPageId);
    TuplePageId tuplePageId = spaceMapPage.findPageWithSpace(requiredSpace);

    // CASE 1: A tuple page with the required space was found.
    if (SpaceMapPage::isValidPage(tuplePageId)) {
      unpinPagePort.unpinPage(spaceMapPageId);
      return {tuplePageId, spaceMapPageId};
    }

    // CASE 2: No suitable tuple page exists on this SpaceMap page.
    SpaceMapPageId nextSpaceMapPageId = spaceMapPage.getNextPageId();

    // CASE 2.1: Continue through the existing SpaceMap chain.
    if (SpaceMapPage::isValidPage(nextSpaceMapPageId)) {
      unpinPagePort.unpinPage(spaceMapPageId);
      spaceMapPageId = nextSpaceMapPageId;
      continue;
    }

    // CASE 2.2: This is the tail of the SpaceMap chain.
    if (!spaceMapPage.isFull()) {
      const CreateTuplePageResult result =
          createTuplePagePort.createTuplePage();
      spaceMapPage.insert(result.pageId, result.freeSpace);
      writePagePort.writePage(spaceMapPageId, spaceMapPage);
      unpinPagePort.unpinPage(spaceMapPageId);
      return {result.pageId, spaceMapPageId};
    }

    // The tail is full: append a SpaceMap page and continue the search there.
    nextSpaceMapPageId = newPagePort.newPage();
    SpaceMapPage newSpaceMapPage;
    writePagePort.writePage(nextSpaceMapPageId, newSpaceMapPage);
    // newPage() returns a pinned page; release that allocation pin before the
    // next loop iteration fetches the page and acquires its own pin.
    unpinPagePort.unpinPage(nextSpaceMapPageId);

    spaceMapPage.setNextPageId(nextSpaceMapPageId);
    writePagePort.writePage(spaceMapPageId, spaceMapPage);
    unpinPagePort.unpinPage(spaceMapPageId);
    spaceMapPageId = nextSpaceMapPageId;
  }

  throw std::runtime_error("No valid SpaceMap page found");
}
