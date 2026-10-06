#pragma once

#include "../../CatalogService/actions/CreateRootCatalogPageAction.h"
#include "../port/CreateRootCatalogPagePort.h"

class CreateRootCatalogPageAdapter : public CreateRootCatalogPagePort {
public:
  explicit CreateRootCatalogPageAdapter(
      CreateRootCatalogPageAction &createRootCatalogPageAction);

  CatalogPageId createRootCatalogPage() override;

private:
  CreateRootCatalogPageAction &createRootCatalogPageAction;
};
