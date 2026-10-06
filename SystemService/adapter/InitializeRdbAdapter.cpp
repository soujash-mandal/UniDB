#include "InitializeRdbAdapter.h"

InitializeRdbAdapter::InitializeRdbAdapter(
    InitializeRdbAction &initializeRdbAction)
    : initializeRdbAction(initializeRdbAction) {}

RdbMetadataPageId InitializeRdbAdapter::initialize() {
  return initializeRdbAction.execute();
}
