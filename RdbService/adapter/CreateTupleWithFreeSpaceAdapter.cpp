#include "CreateTupleWithFreeSpaceAdapter.h"

#include "../../TupleService/actions/CreateTupleWithFreeSpaceAction.h"

#include <utility>

namespace RdbService {

CreateTupleWithFreeSpaceAdapter::CreateTupleWithFreeSpaceAdapter(
    ::CreateTupleWithFreeSpaceAction &action)
    : action(action) {}

RdbCreateTupleWithFreeSpaceResult
CreateTupleWithFreeSpaceAdapter::createTupleWithFreeSpace(
    uint32_t tuplePageId, std::vector<char> tupleData) {
  const CreateTupleWithFreeSpaceActionResult result =
      action.execute(tuplePageId, std::move(tupleData));
  return {result.slotId, result.remainingFreeSpace};
}

} // namespace RdbService
