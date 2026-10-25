#include "CalculateRequiredSpaceAdapter.h"

#include "../../TupleService/actions/CalculateRequiredSpaceAction.h"

namespace RdbService {

CalculateRequiredSpaceAdapter::CalculateRequiredSpaceAdapter(
    CalculateRequiredSpaceAction &action)
    : action(action) {}

uint32_t CalculateRequiredSpaceAdapter::calculateRequiredSpace(
    const std::vector<char> &tupleData) {
  return action.execute(tupleData);
}

} // namespace RdbService
