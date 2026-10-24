# TupleService

TupleService manages tuple-page allocation and tuple-level operations through injected ports; it does not discover which pages belong to a table or coordinate table-wide scans.

## Actions

- `CreateTuplePageAction` — Allocates a new page, initializes it as a tuple page, persists it, and returns its page ID and available free space.
- `CreateTupleAction` — Inserts tuple bytes into an existing tuple page, persists the updated page, and returns the allocated slot ID.
- `GetTupleAction` — Retrieves tuple bytes from a specified page and slot.
- `GetAllTuplesAction` — Retrieves all live tuples from one specified tuple page in slot order and unpins the page; it does not discover table pages.
- `DeleteTupleAction` — Marks the specified tuple slot as deleted and persists the updated page.

## Key domain type

- `TuplePage` — Stores tuple data and slot metadata in an 8 KiB page, validates loaded page bytes, and supports legacy-page migration.

## Boundaries

- Actions depend on TupleService ports; concrete storage behavior is supplied by adapters.
- Tuple pages do not link to one another.
- Table-to-page ownership and table-wide scans belong to the RDB orchestration layer.
