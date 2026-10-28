#include "CreateTupleAdapter.h"
#include "../../TupleService/actions/CreateTupleAction.h"
#include <utility>

CreateTupleAdapter::CreateTupleAdapter(CreateTupleAction &action)
    : action(action) {}

CreateTupleResult
CreateTupleAdapter::createTuple(uint32_t tuplePageId,
                                std::vector<char> tupleData) {
  const auto result = action.execute(tuplePageId, std::move(tupleData));
  return {result.slotId, result.freeSpace};
}
