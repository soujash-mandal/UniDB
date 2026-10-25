#include "GetTuplePageFreeSpaceAdapter.h"

#include "../../TupleService/actions/GetTuplePageFreeSpaceAction.h"

namespace RdbService {

GetTuplePageFreeSpaceAdapter::GetTuplePageFreeSpaceAdapter(
    GetTuplePageFreeSpaceAction &action)
    : action(action) {}

uint32_t GetTuplePageFreeSpaceAdapter::getFreeSpace(uint32_t tuplePageId) {
  return action.execute(tuplePageId);
}

} // namespace RdbService
