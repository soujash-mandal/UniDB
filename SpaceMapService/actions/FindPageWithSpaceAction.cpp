#include "FindPageWithSpaceAction.h"
#include <stdexcept>

FindPageWithSpaceAction::FindPageWithSpaceAction(
    FetchPagePort &fetchPagePort, UnpinPagePort &unpinPagePort,
    WritePagePort &writePagePort, NewPagePort &newPagePort,
    CreateTuplePagePort &createTuplePagePort)
    : fetchPagePort(fetchPagePort), unpinPagePort(unpinPagePort),
      writePagePort(writePagePort), newPagePort(newPagePort),
      createTuplePagePort(createTuplePagePort) {}

TuplePageId FindPageWithSpaceAction::execute(SpaceMapPageId rootSpaceMapPageId,
                                             uint32_t requiredSpace) {

  SpaceMapPageId spaceMapPageId = rootSpaceMapPageId;
  while (SpaceMapPage::isValidPage(spaceMapPageId)) {
    SpaceMapPage spaceMapPage = fetchPagePort.fetchPage(spaceMapPageId);
    TuplePageId tuplePageId = spaceMapPage.findPageWithSpace(requiredSpace);

    // CASE 1: tuple with required space found
    if (SpaceMapPage::isValidPage(tuplePageId)) {
      unpinPagePort.unpinPage(spaceMapPageId);
      return tuplePageId;
    }

    // CASE 2 : Not found
    else {
      SpaceMapPageId nextSpaceMapPageId = spaceMapPage.getNextPageId();

      // CASE 2.1 : valid next page go to next page
      if (SpaceMapPage::isValidPage(nextSpaceMapPageId)) {
        unpinPagePort.unpinPage(spaceMapPageId);
        spaceMapPageId = nextSpaceMapPageId;
      }

      // CASE 2.2 : Next page not exist
      else {

        // CASE  2.2.1 : Current SpaceMap page have space to create tuple page
        if (!spaceMapPage.isFull()) {
          CreateTuplePageResult result = createTuplePagePort.createTuplePage();
          spaceMapPage.insert(result.pageId, result.freeSpace);
          writePagePort.writePage(spaceMapPageId, spaceMapPage);
          unpinPagePort.unpinPage(spaceMapPageId);
          return result.pageId;
        }
        // CASE  2.2.2 : Current SpaceMap page is Full
        else {
          nextSpaceMapPageId = newPagePort.newPage();

          SpaceMapPage newFsmPage;
          writePagePort.writePage(nextSpaceMapPageId, newFsmPage);

          spaceMapPage.setNextPageId(nextSpaceMapPageId);
          writePagePort.writePage(spaceMapPageId, spaceMapPage);
          unpinPagePort.unpinPage(spaceMapPageId);

          spaceMapPageId = nextSpaceMapPageId;
        }
      }
    }
  }
  throw std::runtime_error("No valid SpaceMap page found");
}
