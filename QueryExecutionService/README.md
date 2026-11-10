# QueryExecutionService

QueryExecutionService parses SQL, executes the supported UQL statement subset, and accesses relational data only through RDBService's public Actions.

## Actions

- `ExecuteQueryAction` — executes `CREATE TABLE`, multi-row `INSERT INTO ... VALUES (...), (...)`, and `SELECT` with projections, predicates, grouping, aggregates, ordering, and pagination.

The implementation is intentionally a subset of PostgreSQL 18. The support notes below describe current UniDB behaviour; they are not a claim of general PostgreSQL compatibility.

## Exact NUMERIC / DECIMAL support

`NUMERIC` and `DECIMAL` resolve to the same canonical exact-decimal type. Values are represented using a sign, decimal coefficient, and scale. NUMERIC values do not pass through binary floating-point storage or arithmetic.

| Area | Current support |
|---|---|
| Declarations | `NUMERIC`, `DECIMAL`, and precision/scale modifiers, including negative scale |
| Literals | Signed decimal and exponent notation, parsed without floating-point conversion |
| INSERT / SELECT | Exact round trips through the versioned row codec and catalog type metadata |
| Assignment | Declared precision/scale enforcement; scale rounding is half away from zero; precision overflow is rejected |
| Comparison | Exact equality and ordering comparisons |
| Query operations | Exact `WHERE` comparisons, `IN` / `NOT IN`, inclusive `BETWEEN` / `NOT BETWEEN`, `ORDER BY`, `DISTINCT`, and `GROUP BY` |
| Arithmetic | Exact `+`, `-`, `*`, and `/` expressions; division targets at least 16 significant digits using a magnitude-aware PostgreSQL-style scale rule and rounds half away from zero |
| Aggregates | Exact `SUM`, `AVG`, `MIN`, and `MAX` results |
| Grouped aggregates | `HAVING` comparisons for the selected aggregate, including exact decimal literals and integer literals compared against NUMERIC results |

The implementation keeps SQL type descriptors and runtime values distinct from storage encoding. NUMERIC metadata is carried through the QES/RDB boundary; the tuple layer continues to treat row bytes as opaque.

## Query semantics and current limits

- `AND` has higher precedence than `OR`; parentheses group and nest predicates. Nesting is limited to 128 levels.
- `IN` / `NOT IN` accept non-empty INT, TEXT, or exact NUMERIC literal lists; inclusive `BETWEEN` / `NOT BETWEEN` accept INT, TEXT, or exact NUMERIC bounds. Integer literals are normalized to exact NUMERIC values when compared with NUMERIC columns. Values must otherwise match the target column type.
- SQL `NULL`, three-valued logic, and nullable row values are not supported by this feature set.
- Explicit SQL casts and general PostgreSQL implicit-coercion rules are not implemented. NUMERIC values use a canonical representation that removes insignificant trailing zeroes, so PostgreSQL's original display scale for operands is not retained.
- `ORDER BY` supports source columns and SELECT output aliases with `ASC` / `DESC`. Output aliases take precedence over source-column names when both match.
- Filtering, grouping, aggregate evaluation, and sorting operate in memory on rows returned by the RDB Action.
- Grouped aggregate SELECT supports `ORDER BY` on projected grouping columns and the aggregate output alias, including multiple ordering terms. `HAVING` is evaluated before ordering; `LIMIT` and `OFFSET` are applied afterward.
- `DISTINCT` over grouped aggregate queries remains unsupported.
- Multi-row `INSERT` validates and encodes rows before writes begin, then reuses the existing single-row RDB Action sequentially. Inserts are not transactional: if a later storage operation fails, earlier rows remain committed and the error reports the number inserted.

## Ports and adapters

- `RdbServicePort` exposes the table and row operations QES needs without exposing lower-level storage details.
- `RdbServiceAdapter` maps SQL schemas and rows to RDBService's public Actions.
- QES must not depend directly on CatalogService, TupleService, BufferPoolManager, or DiskManager. Tuple byte encoding must not leak into domain APIs.

## Domain

Infrastructure-independent query and value logic lives under `domain/`:

- `codec/SqlTupleCodec` — pure, stateless encoding and decoding of supported SQL tuple values, including versioned exact NUMERIC values.
- `types/SqlNumericSemantics` — exact NUMERIC parsing, normalization, formatting, comparison, arithmetic, and division primitives.
- `parser/` — converts SQL text into the typed statement representation defined by the contract.
- `predicate/WherePredicateEvaluator` — typed SQL value comparison, WHERE predicate compilation, and recursive predicate evaluation.
- `expression/` — compiles and evaluates supported scalar expressions.
- `aggregate/SqlAggregateCalculator` — calculates COUNT, MIN, MAX, SUM, and AVG over explicit input values.

The parser owns SQL syntax; domain components own pure query/value semantics. Domain code must not depend on services, Actions, ports, adapters, the Container, or storage. Actions remain responsible for query-context orchestration and I/O.

## Parser contract

- `domain/contract/` contains the parser interface and typed SQL statement/value representations.
- The implementation under `domain/parser/` depends on `domain/contract/`. Neither the contract nor parser implementation executes queries or accesses database services.

## Compatibility policy

NUMERIC compatibility is defined per operation and tested independently. The table above is the implemented subset, not a promise that all PostgreSQL 18 NUMERIC syntax, coercions, error messages, or result-scale rules are reproduced. Any additional behaviour must have an explicit rule and regression coverage before it is described as supported.
