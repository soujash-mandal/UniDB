# RDBService

RDBService coordinates relational-table operations through its own Actions,
Ports, and Adapters. Its Actions depend on RDBService-owned contracts; Adapters
translate those contracts to public Actions exposed by lower-level services.

## Actions

- `InitializeRdbAction` — initialize relational database metadata and catalog root.

## Ports

- `CreateRdbMetadataPagePort` — create the RDB metadata page.
- `CreateRootCatalogPagePort` — create the catalog root page.
- `UpdateRdbMetadataPagePort` — persist the catalog root reference in metadata.
- `CatalogTablePort` — create, retrieve, and update table definitions.

## Adapters

- `CreateRdbMetadataPageAdapter` — delegates metadata-page creation.
- `CreateRootCatalogPageAdapter` — delegates catalog-root creation.
- `UpdateRdbMetadataPageAdapter` — delegates metadata updates.
- `CreateCatalogTableAdapter` — translates RDB table definitions for the public
  CatalogService create Action; contains one Action dependency.
- `GetCatalogTableByNameAdapter` — translates CatalogService lookup results to
  the RDB-owned table model; contains one Action dependency.
- `UpdateCatalogTableAdapter` — translates RDB table definitions for the public
  CatalogService update Action; contains one Action dependency.
- `CatalogTablePortFacade` — composes the three single-Action adapters behind
  `CatalogTablePort` and contains no direct Action dependencies.

This service must not call BufferPoolManager, FreePageManager, or other services'
internal ports/adapters directly.
