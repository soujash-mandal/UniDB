#pragma once

#include "../../RdbMetadataService/actions/UpdateRdbMetadataPageAction.h"
#include "../port/UpdateRdbMetadataPagePort.h"

class UpdateRdbMetadataPageAdapter : public UpdateRdbMetadataPagePort {
public:
  explicit UpdateRdbMetadataPageAdapter(
      UpdateRdbMetadataPageAction &updateRdbMetadataPageAction);

  void updateRdbMetadataPage(CatalogPageId pageId,
                             CatalogPageId catalogRootPageId) override;

private:
  UpdateRdbMetadataPageAction &updateRdbMetadataPageAction;
};
