#pragma once

#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../port/GetCatalogRootPagePort.h"

class RdbMetadataGetCatalogRootPageAdapter
    : public GetCatalogRootPagePort {
public:
    RdbMetadataGetCatalogRootPageAdapter(
        GetCatalogRootPageIdAction &getCatalogRootPageIdAction,
        CatalogPageId rdbMetadataPageId);

    CatalogPageId getCatalogRootPageId() override;

private:
    GetCatalogRootPageIdAction &getCatalogRootPageIdAction;
    CatalogPageId rdbMetadataPageId;
};
