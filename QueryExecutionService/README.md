# QueryExecutionService

QueryExecutionService parses SQL, dispatches the V1 statement subset through RDBService's public Actions, and converts logical values to and from tuple bytes.

## Actions

- `ExecuteQueryAction` — execute `CREATE TABLE`, `INSERT INTO ... VALUES (...)`, and `SELECT` with projections, `WHERE` comparisons combined using `AND` / `OR`, and `ORDER BY`, `LIMIT`, and `OFFSET`, returning structured results or errors.

`AND` has higher precedence than `OR`. Parenthesized WHERE expressions are not supported yet. `ORDER BY` supports one or more INT/TEXT columns with `ASC` / `DESC` (ascending by default). WHERE filtering and ORDER BY sorting operate on the rows returned by the existing RDB Action; sorting is performed in memory after filtering. `LIMIT` and `OFFSET` are applied after sorting; `OFFSET` may be used without `LIMIT` and defaults to zero.

## Ports

- `RdbServicePort` — exposes the table and row operations QES needs without exposing lower-level storage details.

## Adapters

- `RdbServiceAdapter` — maps SQL schemas and rows to RDBService's public Actions.
- `SqlTupleCodec` — encodes and decodes V1 INT/TEXT values in tuple storage format.

QES must not depend directly on CatalogService, TupleService, BufferPoolManager, or DiskManager.
