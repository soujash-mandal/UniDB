# QueryExecutionService

QueryExecutionService parses SQL, dispatches the V1 statement subset through RDBService's public Actions, and converts logical values to and from tuple bytes.

## Actions

- `ExecuteQueryAction` — execute `CREATE TABLE`, `INSERT INTO ... VALUES (...), (...)`, and `SELECT` with projections, `WHERE` comparisons, `IN` / `NOT IN` literal lists, inclusive `BETWEEN` / `NOT BETWEEN` ranges, and predicates combined using `AND` / `OR`, plus `ORDER BY`, `LIMIT`, and `OFFSET`, returning structured results or errors.

`AND` has higher precedence than `OR`; parentheses can group and nest conditions to override the default precedence. WHERE expressions are parsed into a tree and evaluated in memory against rows returned by the existing RDB Action. `IN` and `NOT IN` accept non-empty literal lists of INT or TEXT values; list values must match the target column type and are checked before rows are read. `BETWEEN` and `NOT BETWEEN` accept inclusive lower and upper INT or TEXT bounds; both bounds are type-checked before rows are read. Empty IN lists and SQL NULL semantics are not supported. Nesting is limited to 128 parenthesis levels. `ORDER BY` supports one or more INT/TEXT source columns or SELECT output aliases with `ASC` / `DESC` (ascending by default). Output aliases take precedence over source-column names when both match. Aliases can refer to computed projection values such as arithmetic, CASE, and scalar text-function results. WHERE filtering and ORDER BY sorting operate on the rows returned by the existing RDB Action; sorting is performed in memory after filtering. `LIMIT` and `OFFSET` are applied after sorting; `OFFSET` may be used without `LIMIT` and defaults to zero. Multi-row `INSERT` reuses the existing single-row RDB Action for each row. Rows are validated and encoded before writes begin, but inserts are sequential and not transactional: if a later storage operation fails, earlier rows remain committed and the error reports the number inserted.

## Ports

- `RdbServicePort` — exposes the table and row operations QES needs without exposing lower-level storage details.

## Adapters

- `RdbServiceAdapter` — maps SQL schemas and rows to RDBService's public Actions.
- `SqlTupleCodec` — encodes and decodes V1 INT/TEXT values in tuple storage format.

## Domain

Pure, stateless SQL/query rules live under `domain/` and receive their inputs explicitly:

- `predicate/WherePredicateEvaluator` — typed SQL value comparison, WHERE predicate compilation, and recursive predicate evaluation.
- `expression/ScalarExpressionEvaluator` — evaluates compiled integer arithmetic and TEXT scalar functions.
- `aggregate/SqlAggregateCalculator` — calculates COUNT, MIN, MAX, SUM, and AVG over explicit input values.

Domain code must not depend on services, Actions, ports, adapters, the Container, or storage. Schema lookup, column/alias binding, join resolution, row fetching, grouping, HAVING, sorting, pagination, and result construction remain in `ExecuteQueryAction` because they depend on query context or coordinate execution.

## Parser ownership

- `contract/` contains the parser interface and typed SQL statement/value representations.
- `parser/` contains the lexer, parser implementation, and parser unit tests.

The parser implementation depends on the contract. Neither parser layer executes queries or accesses database services.

## Service boundary

QES accesses RDB only through `RdbServicePort`, backed by the existing public RDBService Actions. QES must not depend directly on CatalogService, TupleService, BufferPoolManager, or DiskManager. Tuple byte encoding remains in the QES adapter and must not leak into domain APIs.

Grouped aggregate SELECT supports in-memory `ORDER BY` on projected grouping columns and the aggregate output alias, including multiple ASC/DESC terms. `LIMIT` and `OFFSET` are applied after HAVING and ordering. `DISTINCT` remains unsupported for grouped aggregate queries.
