#pragma once

#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../port/GetCatalogRootPageIdPort.h"

class GetCatalogRootPageIdAdapter final : public GetCatalogRootPageIdPort {
public:
  explicit GetCatalogRootPageIdAdapter(GetCatalogRootPageIdAction &action);
  uint32_t getCatalogRootPageId(uint32_t metadataPageId) override;

private:
  GetCatalogRootPageIdAction &action;
};
