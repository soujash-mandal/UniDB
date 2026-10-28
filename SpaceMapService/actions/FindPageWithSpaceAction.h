#pragma once

#include "../domain/SpaceMapPage.h"
#include "../port/CreateTuplePagePort.h"
#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/SpaceMapWritePagePort.h"

struct FindPageWithSpaceResult {
  TuplePageId tuplePageId;
  SpaceMapPageId spaceMapPageId;
};

class FindPageWithSpaceAction {
public:
  FindPageWithSpaceAction(FetchPagePort &fetchPagePort,
                          UnpinPagePort &unpinPagePort,
                          SpaceMapWritePagePort &writePagePort,
                          NewPagePort &newPagePort,
                          CreateTuplePagePort &createTuplePagePort);

  FindPageWithSpaceResult execute(SpaceMapPageId rootSpaceMapPageId,
                                  uint32_t requiredSpace);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
  SpaceMapWritePagePort &writePagePort;
  NewPagePort &newPagePort;
  CreateTuplePagePort &createTuplePagePort;
};
