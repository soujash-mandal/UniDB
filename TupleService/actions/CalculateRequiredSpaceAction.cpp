#include "CalculateRequiredSpaceAction.h"

#include "../domain/TuplePage.h"

#include <stdexcept>

uint32_t CalculateRequiredSpaceAction::execute(
    const std::vector<char> &tupleData) const {
  constexpr std::size_t MAX_TUPLE_BYTES =
      TUPLE_PAGE_SIZE - sizeof(TuplePageHeader) - sizeof(TupleSlotEntry);
  if (tupleData.size() > MAX_TUPLE_BYTES) {
    throw std::length_error("Tuple exceeds the V1 tuple size limit");
  }

  return static_cast<uint32_t>(tupleData.size() + sizeof(TupleSlotEntry));
}
