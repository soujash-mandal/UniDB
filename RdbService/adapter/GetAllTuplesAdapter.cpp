#include "GetAllTuplesAdapter.h"
#include "../../TupleService/actions/GetAllTuplesAction.h"
GetAllTuplesAdapter::GetAllTuplesAdapter(GetAllTuplesAction &action) : action(action) {}
std::vector<std::vector<char>> GetAllTuplesAdapter::getAllTuples(uint32_t tuplePageId) {
  return action.execute(tuplePageId);
}
