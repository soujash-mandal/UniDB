#include "SpaceMapCreateTuplePageAdapter.h"

SpaceMapCreateTuplePageAdapter::SpaceMapCreateTuplePageAdapter(
    CreateTuplePageAction &createTuplePageAction)
    : createTuplePageAction(createTuplePageAction) {}

CreateTuplePageResult SpaceMapCreateTuplePageAdapter::createTuplePage() {
  CreateTuplePageActionResult result = createTuplePageAction.execute();
  return {result.pageId, result.freeSpace};
}
