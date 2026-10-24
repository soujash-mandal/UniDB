#include "GetTupleAdapter.h"
#include "../../TupleService/actions/GetTupleAction.h"
GetTupleAdapter::GetTupleAdapter(GetTupleAction &action) : action(action) {}
std::vector<char> GetTupleAdapter::getTuple(uint32_t tuplePageId, uint32_t tupleSlotId) {
  return action.execute(tuplePageId, tupleSlotId);
}
