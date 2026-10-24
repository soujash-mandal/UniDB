#include "CreateTuplePageAdapter.h"
#include "../../TupleService/actions/CreateTuplePageAction.h"
CreateTuplePageAdapter::CreateTuplePageAdapter(CreateTuplePageAction &action)
    : action(action) {}
RdbCreatedTuplePage CreateTuplePageAdapter::createTuplePage() {
  const auto result = action.execute();
  return {result.pageId, result.freeSpace};
}
