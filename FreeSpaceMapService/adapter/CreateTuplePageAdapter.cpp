#include "CreateTuplePageAdapter.h"
#include <cstdint>

CreateTuplePageAdapter::CreateTuplePageAdapter(
    CreateTuplePageAction &createTuplePageAction)
    : createTuplePageAction(createTuplePageAction) {}

uint32_t CreateTuplePageAdapter::createTuplePage() {
  return createTuplePageAction.execute();
}
