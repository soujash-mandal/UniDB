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
- `SpaceMapPort` — create a SpaceMap page, locate a page with sufficient free space, and enumerate a table's tuple pages.
- `TuplePagePort` — create tuple pages and create/read tuples.

## Adapters

- `CreateRdbMetadataPageAdapter` — delegates metadata-page creation.
- `CreateRootCatalogPageAdapter` — delegates catalog-root creation.
- `UpdateRdbMetadataPageAdapter` — delegates metadata updates.
- `CatalogTableAdapter` — translates RDB-owned table definitions to and from CatalogService public Actions.
- `SpaceMapAdapter` — delegates page discovery and SpaceMap operations to SpaceMapService public Actions.
- `TuplePageAdapter` — delegates tuple-page and row operations to TupleService public Actions.

This service must not call BufferPoolManager, FreePageManager, or other services'
internal ports/adapters directly.
