# PostgreSQL-Compatible SQL Type System — Design Proposal

> **Status: DRAFT — NOT APPROVED**
>
> This document is the first design deliverable for SIM-81. It proposes boundaries and a staged model for review. It does not authorize source-code changes, row-format changes, catalog migrations, or PostgreSQL-compatibility claims.

## 1. Goal and compatibility target

Evolve UniDB's SQL type system toward PostgreSQL 18 built-in type semantics without turning UniDB into a PostgreSQL server.

Compatibility is defined per type and per operation, not as a blanket claim. A type is considered supported only when its parsing, value representation, validation, coercion, comparison, execution, storage, result rendering, and relevant error behaviour are specified and tested.

The initial type families in SIM-81 are:

- Integers: SMALLINT / INT2, INTEGER / INT / INT4, BIGINT / INT8.
- Floating point: REAL / FLOAT4, DOUBLE PRECISION / FLOAT8.
- Exact numeric: NUMERIC / DECIMAL.
- Boolean: BOOLEAN / BOOL.
- Character: CHAR(n), VARCHAR(n), TEXT.
- Date/time: DATE, TIME, TIME WITH TIME ZONE, TIMESTAMP, TIMESTAMP WITH TIME ZONE, INTERVAL.
- Other core types: BYTEA, UUID, JSON, JSONB.

Arrays, ENUM, network, geometric, range, and other PostgreSQL-specific families remain future extensions.

## 2. Verified current baseline

The current main-branch implementation has several distinct representations that must not be conflated:

1. `QueryExecutionService/type/type.h` declares `SqlType { Int, Text }`, `SqlValue = variant<int64_t, string>`, and SQL column/result types.
2. `QueryExecutionService/domain/contract/sqlStatement.h` uses those SQL types and values in parsed statements.
3. `QueryExecutionService/domain/parser/SqlParserImpl.cpp` currently parses only INT and TEXT column declarations and signed 64-bit integer / single-quoted text literals. NULL, TRUE, and FALSE are rejected as unsupported literals.
4. `QueryExecutionService/domain/codec/SqlTupleCodec.cpp` encodes INT as an eight-byte little-endian signed integer and TEXT as a four-byte little-endian length followed by bytes. The V1 codec has no row header, NULL bitmap, or format marker.
5. `CatalogService/domain/CatalogTypes.h` has its own `DataType` enum (INT, BIGINT, FLOAT, DOUBLE, BOOLEAN, VARCHAR) and a column size/nullable field.
6. `RdbService/domain/RdbTable.h` has a separate RDB-owned `RdbDataType` enum with similar values. The QES adapter currently maps only SQL INT and TEXT into the RDB model.
7. Catalog and RDB type enums therefore do not yet represent the full SQL type system, and SQL type identity is not currently persisted as a complete canonical type plus modifiers.

Legacy database compatibility is explicitly out of scope. UniDB may change the meaning and physical width of existing SQL types, replace the row format, and require existing database files to be recreated. This is acceptable because there are no existing users or user databases that need to be preserved.

## 3. Proposed conceptual model

Keep three concepts separate:

### 3.1 SQL type identity

A SQL type describes the declared type and its modifiers, independent of a particular value or storage encoding.

Proposed descriptor fields:

- Canonical type identifier (for example, Integer, BigInt, Numeric, Varchar, TimestampTz).
- Declared modifier data where applicable (for example, character length, NUMERIC precision and scale, temporal precision).
- Original spelling is optional diagnostic/parser information; aliases resolve to the same canonical identity.

Aliases are parser/catalog metadata, not separate runtime types. Examples: INT and INT4 resolve to INTEGER; BOOL resolves to BOOLEAN; DECIMAL resolves to NUMERIC.

### 3.2 Runtime SQL values

A runtime value carries a value of a concrete runtime representation, independently of how a table row stores it.

The value model must preserve distinctions that affect SQL semantics. In particular:

- SMALLINT, INTEGER, and BIGINT must have their PostgreSQL ranges and must not silently share an unbounded signed-64-bit semantic type.
- REAL and DOUBLE PRECISION must preserve their respective IEEE-754 widths.
- NUMERIC must not be implemented by converting through `double` or `long double`; exact decimal precision and scale require an exact representation.
- BOOLEAN must be a real boolean value, not integer 0/1.
- DATE/TIME/TIMESTAMP/INTERVAL values need explicit, documented temporal representations and timezone rules.
- BYTEA must preserve arbitrary bytes, including zero bytes.
- UUID must have a validated 128-bit representation.
- JSON and JSONB must have explicitly different equality, normalization, and storage semantics where PostgreSQL differs.
- SQL NULL is a separate value state, not a declared SQL type. Its design remains coordinated with SIM-78.

Do not choose a final C++ variant layout until the numeric, temporal, JSON, and NULL dependencies have been evaluated. Wrapper structs may be required to avoid ambiguous alternatives and to preserve type-specific semantics.

### 3.3 Physical row encoding

The row codec is a persistence boundary. It must encode a declared schema and runtime values without deciding SQL coercion rules.

The codec must use an explicit database/format version so UniDB can identify which layout it is reading. It must reject malformed, truncated, incompatible, or unsupported data rather than guessing. Since backward compatibility is not required, the implementation may replace the current V1 row format instead of retaining a legacy decoder. Format versioning is still required for safe future evolution.

## 4. Type registry and ownership proposal

The type registry is pure SQL-domain logic. It should resolve canonical names and aliases, validate modifiers, expose type properties, and provide explicit conversion/comparison rules. It must not perform catalog I/O, tuple I/O, or service calls.

Proposed ownership:

| Area | Responsibility |
|---|---|
| SQL parser / SQL contract | Parse type names, aliases, modifiers, typed literals and casts into SQL-owned descriptors/expressions. |
| QES type declarations | Own SQL type/value/column/result declarations in its isolated type folder; declarations must not import other project headers. |
| QES domain | Pure type resolution, validation, coercion, comparison, arithmetic and expression semantics; query execution orchestration remains in Actions. |
| QES adapter | Translate SQL-owned schema/value representations to and from the RDB public Action-backed interface. No direct CatalogService or TupleService imports in QES. |
| RDBService | Own its public table metadata model and Actions. Translate its model through its own adapters; do not expose CatalogService internal types. |
| CatalogService | Persist catalog-owned canonical type identifiers and modifiers using a deliberate, versioned catalog format. Existing catalog payload compatibility is not required. |
| TupleService | Continue storing opaque row bytes; it must not interpret SQL types or SQL values. |
| Container | Wire the existing Actions and adapters; no type semantics should live in the composition root. |

Avoid a shared cross-service header that causes QES, RDBService, and CatalogService to import one another's domain types. Each service owns its boundary model; adapters perform explicit translation.

## 5. Type descriptor and alias rules

The design should use one canonical identity per SQL type. Alias parsing must be case-insensitive and normalize whitespace where PostgreSQL permits alternate spellings.

Examples to specify:

| Input spelling | Canonical identity | Notes |
|---|---|---|
| SMALLINT, INT2 | SMALLINT | signed 16-bit range |
| INTEGER, INT, INT4 | INTEGER | signed 32-bit range |
| BIGINT, INT8 | BIGINT | signed 64-bit range |
| REAL, FLOAT4 | REAL | IEEE-754 binary32 |
| DOUBLE PRECISION, FLOAT8 | DOUBLE PRECISION | IEEE-754 binary64 |
| NUMERIC, DECIMAL | NUMERIC | exact decimal; modifiers require design |
| BOOLEAN, BOOL | BOOLEAN | true/false |
| CHARACTER(n), CHAR(n) | CHAR | fixed-width character semantics |
| CHARACTER VARYING(n), VARCHAR(n) | VARCHAR | bounded character semantics |
| TEXT | TEXT | unbounded character type |

Type modifiers must be validated at declaration time and persisted in catalog metadata. Invalid modifier values must fail clearly rather than being silently ignored.

## 6. Coercion, comparison, and expression rules

A conversion matrix must be defined before implementation. It must distinguish:

- Assignment coercion (INSERT into a declared column).
- Explicit casts.
- Implicit expression coercion.
- Literal type inference.
- Comparison coercion.
- Arithmetic result type and overflow.
- Aggregate input/result types.
- Sorting and grouping equality/order semantics.

Do not implement a generic "convert everything to string" or "convert everything to double" fallback.

For each supported pair, the matrix must say whether conversion is implicit, explicit-only, or rejected, and define precision/range loss behaviour. Integer arithmetic must detect overflow according to the chosen PostgreSQL-compatible operation semantics. Floating-point behaviour must explicitly cover NaN, infinities, signed zero, and comparisons. NUMERIC must define scale propagation, rounding, and overflow.

Character semantics must specify encoding assumptions, character-count versus byte-count limits, CHAR padding and trailing-space comparisons. Temporal semantics must specify precision, timezone handling, session timezone assumptions, and daylight-saving behaviour. JSON and JSONB must define parsing, equality, ordering support, and canonicalization independently.

## 7. Persistence and format versioning

Backward compatibility with existing UniDB database files is explicitly not a requirement for this project stage. We can make a clean break and require databases created by older builds to be deleted and recreated.

Required invariants for the new format:

1. New rows use PostgreSQL-compatible physical widths for fixed-width types: SMALLINT is 2 bytes, INTEGER is 4 bytes, and BIGINT is 8 bytes.
2. New type metadata is persisted as a canonical type identity plus validated modifiers, not as an enum ordinal whose meaning can change.
3. The database or catalog has an explicit format version. The reader checks it before interpreting persisted metadata or rows.
4. Unknown type identifiers, unknown format versions, malformed rows, and truncated data fail clearly; the reader must never guess a layout.
5. No legacy V1 decoder, catalog migration, or automatic rewrite of old database files is required. Document the clean-recreation requirement for developers and users.
6. Future format changes must increment or otherwise explicitly evolve the format version and provide a deliberate compatibility policy.

SIM-78 still owns the NULL and nullable-row design. Its row header/versioning and NULL representation decisions must be coordinated with SIM-81 before implementation. SIM-81 should consume that design rather than create a competing row format.

## 8. Phased implementation proposal

The following sequence is provisional and should be split into small owning-service tickets after this design is approved:

1. **Foundation:** finalize canonical type identifiers, modifiers, aliases, SQL value representation, type registry API, and catalog/RDB translation contracts. No row-format change in this phase.
2. **Integer family:** implement PostgreSQL-compatible SMALLINT, INTEGER, and BIGINT ranges and physical widths directly. Existing SQL INT storage is allowed to change; no legacy mapping or migration is needed.
3. **BOOLEAN:** typed literals, predicates, storage, result formatting and tests; coordinate NULL/three-valued logic with SIM-78.
4. **REAL and DOUBLE PRECISION:** parsing, range/formatting, arithmetic and comparison semantics.
5. **NUMERIC:** exact decimal representation, precision/scale, arithmetic, casts, rounding and overflow.
6. **Character family:** CHAR(n), VARCHAR(n), TEXT and length/padding/collation-related semantics.
7. **Temporal family:** DATE, TIME, TIMESTAMP, timezone-aware values and INTERVAL.
8. **BYTEA and UUID.**
9. **JSON and JSONB.**
10. **Compatibility audit:** PostgreSQL 18 comparison tests, documented deviations, format-safety tests and end-to-end SQL coverage.

Do not treat the family list as a fixed PR count. The approved design and dependency graph should determine the actual ticket/PR boundaries.

## 9. Test strategy

Every type family needs tests at multiple boundaries:

- Parser: canonical names, aliases, modifiers, typed literals, invalid syntax.
- Type domain: ranges, precision, scale, casts, coercion matrix, comparisons, arithmetic, sorting and error cases.
- Catalog: metadata round trips, unknown type identifiers, invalid modifiers, and explicit format-version handling.
- Codec: encode/decode round trips, exact byte fixtures, malformed/truncated data, wrong schema, unknown format version.
- QES integration: CREATE TABLE, INSERT, SELECT, predicates, JOIN where supported, ORDER BY, GROUP BY and aggregates where applicable.
- Format safety: new-format fixtures verify version checks, deterministic decoding, and clear rejection of unknown versions or malformed data. Existing database fixtures do not need to remain readable.
- Differential tests: execute equivalent supported cases against PostgreSQL 18 and UniDB, compare results and expected errors. Record intentional differences.

A PostgreSQL comparison test must be reproducible and should record server version, SQL input, expected output/error, and UniDB output/error.

## 10. Decisions required before implementation

The following decisions are intentionally left open for review:

1. What is the canonical C++ SQL type descriptor shape, including modifier representation?
2. Which exact runtime value wrappers are needed, and which decimal/date-time/JSON libraries are acceptable?
3. What is the SQL contract boundary for parsed types and values while preserving the isolated type-folder rule?
4. How should canonical type identifiers and modifiers be persisted without relying on unstable enum ordinals?
5. Confirm the clean-break type mapping: existing SQL INT becomes PostgreSQL INTEGER (32-bit); BIGINT is a separate 64-bit type.
6. Which new table/row format versioning decision is approved by SIM-78?
7. What is the supported character encoding and collation baseline?
8. What timezone and session-timezone behaviour is in scope for the first temporal release?
9. Which PostgreSQL 18 behaviours are required for each type before it can be called compatible?
10. What exact database/row format version marker and clean-start policy should the first new format use?

## 11. Approval gate

This proposal is ready for architectural review, not implementation.

Implementation may begin only after:
- Soujash approves the type/value/registry and service-ownership design.
- The clean-break policy is accepted: old database files may be discarded and recreated.
- SIM-78's row-format and NULL design is approved wherever the implementation depends on it.
- The first implementation ticket defines one bounded capability and its tests.

Until then, no SQL type enum expansion, parser feature, catalog schema change, tuple codec change, or persisted database format change is authorized by this document.
