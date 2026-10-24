# RDBService

RDBService coordinates relational-table operations through its own Actions,
Ports, and Adapters. Its Actions depend on RDBService-owned contracts; Adapters
translate those contracts to public Actions exposed by lower-level services.

## Actions

- `InitializeRdbAction` — initialize relational database metadata and catalog root.
- `RdbCreateTableAction` — validate and persist a table definition.
- `GetTableDefinitionAction` — retrieve a table definition by name.
- `SelectAllRowsAction` — read and decode rows across all tuple pages for a table.

## Ports

- `CreateRdbMetadataPagePort` — create the RDB metadata page.
- `CreateRootCatalogPagePort` — create the catalog root page.
- `UpdateRdbMetadataPagePort` — persist the catalog root reference in metadata.
- `RdbMetadataPort` — retrieve the catalog root and allocate table IDs.
- `CatalogTablePort` — create, retrieve, and update table definitions.
- `SpaceMapPort` — create a SpaceMap page, locate a page with sufficient free space, and enumerate a table's tuple pages.
- `TuplePagePort` — create tuple pages and create/read tuples.

## Adapters

- `CreateRdbMetadataPageAdapter` — delegates metadata-page creation.
- `CreateRootCatalogPageAdapter` — delegates catalog-root creation.
- `UpdateRdbMetadataPageAdapter` — delegates metadata updates.
- `RdbMetadataAdapter` — delegates metadata reads and table-ID allocation to RdbMetadataService public Actions.
- `CatalogTableAdapter` — translates RDB-owned table definitions to and from CatalogService public Actions.
- `SpaceMapAdapter` — delegates page discovery and SpaceMap operations to SpaceMapService public Actions.
- `TuplePageAdapter` — delegates tuple-page and row operations to TupleService public Actions.

`RdbRowCodec` owns the V1 row byte format; it supports INT, BIGINT, and VARCHAR
columns for the current SQL subset. Unsupported types fail explicitly.

This service must not call BufferPoolManager, FreePageManager, or other services'
internal ports/adapters directly.
