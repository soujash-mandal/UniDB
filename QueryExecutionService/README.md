# QueryExecutionService

QueryExecutionService parses SQL, dispatches the V1 statement subset through RDBService's public Actions, and converts logical values to and from tuple bytes.

## Actions

- `ExecuteQueryAction` — execute `CREATE TABLE`, `INSERT INTO ... VALUES (...)`, and `SELECT *` / `SELECT column, ...` with `WHERE` comparisons combined using `AND` / `OR`, returning structured results or errors.

`AND` has higher precedence than `OR`. Parenthesized WHERE expressions are not supported yet. WHERE evaluation currently scans all rows returned by the existing RDB Action.

## Ports

- `RdbServicePort` — exposes the table and row operations QES needs without exposing lower-level storage details.

## Adapters

- `RdbServiceAdapter` — maps SQL schemas and rows to RDBService's public Actions.
- `SqlTupleCodec` — encodes and decodes V1 INT/TEXT values in tuple storage format.

QES must not depend directly on CatalogService, TupleService, BufferPoolManager, or DiskManager.
