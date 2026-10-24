#include "CreateTupleAdapter.h"
#include "../../TupleService/actions/CreateTupleAction.h"
#include <utility>
CreateTupleAdapter::CreateTupleAdapter(CreateTupleAction &action) : action(action) {}
uint32_t CreateTupleAdapter::createTuple(uint32_t tuplePageId, std::vector<char> tupleData) {
  return action.execute(tuplePageId, std::move(tupleData));
}
