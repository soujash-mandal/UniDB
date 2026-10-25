# RDBService

RDBService coordinates relational-table operations through its own Actions,
Ports, and Adapters. Its Actions depend on RDBService-owned contracts; Adapters
translate those contracts to public Actions exposed by lower-level services.

## Actions

- `InitializeRdbAction` — initialize relational database metadata and catalog root.
- `RdbService::CreateTableAction` — validate a table definition, obtain metadata-backed identifiers through RDB-owned boundaries, and persist it through CatalogService.
- `RdbService::GetTableDefinitionAction` — resolve the catalog root and return a table schema through the RDB-owned catalog lookup boundary.
- `RdbService::SelectAllRowsAction` — read live rows from every tuple page listed by SpaceMap for a table.
- `RdbService::InsertRowAction` — initialize a table's SpaceMap root when needed, locate a tuple page with enough space, and insert one encoded row.

## Ports

- `CreateRdbMetadataPagePort` — create the RDB metadata page.
- `CreateRootCatalogPagePort` — create the catalog root page.
- `UpdateRdbMetadataPagePort` — persist the catalog root reference in metadata.
- `CreateCatalogTablePort` — create a table definition through CatalogService.
- `GetCatalogTableByNamePort` — retrieve a table definition by name.
- `UpdateCatalogTablePort` — update a table definition and its explicit space-map reference.
- `CreateSpaceMapPagePort` — create a table's root space-map page.
- `FindPageWithSpacePort` — locate a tuple page with sufficient free space.
- `GetTablePagesPort` — enumerate tuple pages registered for a table.
- `RdbCreateTuplePagePort` — create and initialize one tuple page.
- `CreateTuplePort` — insert one row into a tuple page.
- `CalculateRequiredSpacePort` — calculate the bytes needed for a tuple payload and its slot entry.
- `UpdateFreeSpacePort` — update a tuple page's recorded free space in SpaceMapService.
- `GetTuplePageFreeSpacePort` — retrieve a tuple page's actual remaining free space.
- `GetTuplePort` — retrieve one row by slot.
- `GetAllTuplesPort` — scan live rows from one tuple page.

## Adapters

- `CreateRdbMetadataPageAdapter` — delegates metadata-page creation.
- `CreateRootCatalogPageAdapter` — delegates catalog-root creation.
- `UpdateRdbMetadataPageAdapter` — delegates metadata updates.
- `CreateCatalogTableAdapter` — maps RDB table definitions to the CatalogService create Action; implements one port and depends on one Action.
- `GetCatalogTableByNameAdapter` — maps CatalogService lookup results to the RDB-owned table model; implements one port and depends on one Action.
- `UpdateCatalogTableAdapter` — maps RDB table definitions to the CatalogService update Action; implements one port and depends on one Action.
- `CreateSpaceMapPageAdapter` — delegates only to `CreateSpaceMapPageAction`.
- `FindPageWithSpaceAdapter` — delegates only to `FindPageWithSpaceAction`.
- `GetTablePagesAdapter` — delegates only to `GetTablePagesAction`.
- `CreateTuplePageAdapter` — delegates only to `CreateTuplePageAction`.
- `CreateTupleAdapter` — delegates only to `CreateTupleAction`.
- `CalculateRequiredSpaceAdapter` — delegates only to `CalculateRequiredSpaceAction`.
- `UpdateFreeSpaceAdapter` — delegates only to `UpdateFreeSpaceAction`.
- `GetTuplePageFreeSpaceAdapter` — delegates only to `GetTuplePageFreeSpaceAction`.
- `GetTupleAdapter` — delegates only to `GetTupleAction`.
- `GetAllTuplesAdapter` — delegates only to `GetAllTuplesAction`.

Each operation-specific Port defines one operation, and each adapter implements exactly one
Port while depending on exactly one corresponding lower-level service public Action.

This service must not call BufferPoolManager, FreePageManager, or other services'
internal ports/adapters directly.
