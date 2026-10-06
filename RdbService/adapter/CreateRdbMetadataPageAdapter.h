#pragma once

#include "../../RdbMetadataService/actions/CreateRdbMetadataPageAction.h"
#include "../port/CreateRdbMetadataPagePort.h"

class CreateRdbMetadataPageAdapter : public CreateRdbMetadataPagePort {
public:
  explicit CreateRdbMetadataPageAdapter(
      CreateRdbMetadataPageAction &createRdbMetadataPageAction);

  CatalogPageId createRdbMetadataPage() override;

private:
  CreateRdbMetadataPageAction &createRdbMetadataPageAction;
};
