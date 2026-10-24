#pragma once

#include "../../RdbMetadataService/actions/UpdateRdbMetadataPageAction.h"
#include "../port/UpdateRdbMetadataPagePort.h"

class UpdateRdbMetadataPageAdapter : public UpdateRdbMetadataPagePort {
public:
  explicit UpdateRdbMetadataPageAdapter(
      UpdateRdbMetadataPageAction &updateRdbMetadataPageAction);

  void updateRdbMetadataPage(MetadataPageId pageId,
                             CatalogPageId catalogRootPageId) override;

private:
  UpdateRdbMetadataPageAction &updateRdbMetadataPageAction;
};
