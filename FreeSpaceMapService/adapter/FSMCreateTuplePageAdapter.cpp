#include "FSMCreateTuplePageAdapter.h"

FSMCreateTuplePageAdapter::FSMCreateTuplePageAdapter(
    CreateTuplePageAction &createTuplePageAction)
    : createTuplePageAction(createTuplePageAction) {}

CreateTuplePageResult FSMCreateTuplePageAdapter::createTuplePage() {
  CreateTuplePageActionResult result = createTuplePageAction.execute();
  return {result.pageId, result.freeSpace};
}
