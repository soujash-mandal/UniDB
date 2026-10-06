#include "CreateRootCatalogPageAdapter.h"

CreateRootCatalogPageAdapter::CreateRootCatalogPageAdapter(
    CreateRootCatalogPageAction &createRootCatalogPageAction)
    : createRootCatalogPageAction(createRootCatalogPageAction) {}

CatalogPageId CreateRootCatalogPageAdapter::createRootCatalogPage() {
  return createRootCatalogPageAction.execute();
}
