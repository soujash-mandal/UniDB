#include "TuplePageAdapter.h"
#include <utility>
TuplePageAdapter::TuplePageAdapter(CreateTuplePageAction &createTuplePageAction,
                                   CreateTupleAction &createTupleAction,
                                   GetTupleAction &getTupleAction,
                                   GetAllTuplesAction &getAllTuplesAction)
    : createTuplePageAction(createTuplePageAction),
      createTupleAction(createTupleAction), getTupleAction(getTupleAction),
      getAllTuplesAction(getAllTuplesAction) {}
RdbCreatedTuplePage TuplePageAdapter::createTuplePage() {
  const auto result = createTuplePageAction.execute();
  return {result.pageId, result.freeSpace};
}
uint32_t TuplePageAdapter::createTuple(uint32_t tuplePageId,
                                       std::vector<char> tupleData) {
  return createTupleAction.execute(tuplePageId, std::move(tupleData));
}
std::vector<char> TuplePageAdapter::getTuple(uint32_t tuplePageId,
                                             uint32_t tupleSlotId) {
  return getTupleAction.execute(tuplePageId, tupleSlotId);
}
std::vector<std::vector<char>>
TuplePageAdapter::getAllTuples(uint32_t tuplePageId) {
  return getAllTuplesAction.execute(tuplePageId);
}