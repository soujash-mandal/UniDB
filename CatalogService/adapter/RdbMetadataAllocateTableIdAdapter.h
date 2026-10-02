#pragma once

#include "../../RdbMetadataService/actions/AllocateNextTableIdAction.h"
#include "../port/AllocateTableIdPort.h"

class RdbMetadataAllocateTableIdAdapter
    : public AllocateTableIdPort {
public:
    RdbMetadataAllocateTableIdAdapter(
        AllocateNextTableIdAction &allocateNextTableIdAction,
        CatalogPageId rdbMetadataPageId);

    CatalogTableId allocateTableId() override;

private:
    AllocateNextTableIdAction &allocateNextTableIdAction;
    CatalogPageId rdbMetadataPageId;
};
