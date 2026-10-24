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
- `CreateCatalogTablePort` — create a table definition through CatalogService.
- `GetCatalogTableByNamePort` — retrieve a table definition by name.
- `UpdateCatalogTablePort` — update a table definition and its explicit space-map reference.
- `SpaceMapPort` — create a table's space-map page, locate a page with sufficient free space, and enumerate tuple pages.
- `TuplePagePort` — create tuple pages and perform row insert, lookup, scan, and delete operations.

## Adapters

- `CreateRdbMetadataPageAdapter` — delegates metadata-page creation.
- `CreateRootCatalogPageAdapter` — delegates catalog-root creation.
- `UpdateRdbMetadataPageAdapter` — delegates metadata updates.
- `CreateCatalogTableAdapter` — maps RDB table definitions to the CatalogService create Action; implements one port and depends on one Action.
- `GetCatalogTableByNameAdapter` — maps CatalogService lookup results to the RDB-owned table model; implements one port and depends on one Action.
- `UpdateCatalogTableAdapter` — maps RDB table definitions to the CatalogService update Action; implements one port and depends on one Action.
- `SpaceMapAdapter` — delegates to public SpaceMapService Actions.
- `TuplePageAdapter` — delegates to public TupleService Actions.

Each operation-specific Port defines one operation, and each adapter implements exactly one
Port while depending on the corresponding lower-level service's public Action.

This service must not call BufferPoolManager, FreePageManager, or other services'
internal ports/adapters directly.
