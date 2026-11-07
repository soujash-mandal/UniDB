# ExecuteQueryAction Responsibility Audit — SIM-80

**Audit target:** `main` at the repository head inspected for SIM-80 (2026-11-07)  
**Scope:** `QueryExecutionService/actions/ExecuteQueryAction.h/.cpp`, its RDB port/adapter boundary, QES domain components, and Container wiring.  
**Change type:** Documentation only. This audit does not change runtime behavior or SQL semantics.

## 1. Executive summary

`ExecuteQueryAction.cpp` is approximately 2,267 lines. It is not a thin Action: it currently contains SQL statement validation, schema/name resolution, query planning, join algorithms, UNION combination, grouping, DISTINCT, ordering, pagination, projection, tuple serialization calls, database I/O coordination, and result/error construction.

The Action has two constructor dependencies:

- `RdbServicePort&`: required I/O boundary for table creation, schema reads, row insertion, and row reads.
- `const unidb::sql::SqlParser&`: required pure parsing boundary; this is not an I/O dependency.

The Action also directly includes and calls `SqlTupleCodec` from the adapter layer. This is a boundary leak: the Action currently knows that rows are represented as tuple bytes. The RDB port should exchange SQL-level values; the adapter should own conversion between SQL values and persisted tuple bytes.

The target is not to move every line out of the Action indiscriminately. The Action should retain execution sequencing, the calls to the required ports, and conversion of exceptions/parse outcomes into the public result type. Pure SQL rules and pure relational operations should be independently testable in QES Domain.

## 2. Current dependency audit

| Dependency | Current use | Decision |
| --- | --- | --- |
| `RdbServicePort` | Create table, fetch schema, insert encoded rows, fetch encoded rows | Keep. This is the Action's required external I/O boundary. |
| `unidb::sql::SqlParser` | Parse SQL text into a typed statement | Keep. It is a pure parser contract, not I/O. |
| `SqlTupleCodec` | Encode INSERT values and decode rows in SELECT/JOIN paths | Remove direct Action dependency. Move the codec calls behind `RdbServiceAdapter` and make the port exchange typed SQL rows/values. |
| Standard C++ algorithms/containers | Currently used throughout query algorithms in the Action | Keep only where needed for orchestration/result handling; move algorithm implementations into focused Domain components. |

### Required I/O operations

The Action currently needs only these RDB capabilities:

1. Create a table.
2. Read a table definition.
3. Insert a row.
4. Read all rows for a table.

These remain behind the existing `RdbServicePort`. Do not add direct CatalogService, TupleService, BufferPoolManager, DiskManager, or storage dependencies.

### Port/adapter boundary finding

Current `RdbServicePort` exposes `std::vector<char>` tuple bytes for insert and select. This forces `ExecuteQueryAction` to call `SqlTupleCodec` directly. Proposed boundary:

- Action/port exchange `SqlType` and `SqlValue` data.
- `RdbServiceAdapter` uses the supplied schema types and values to encode/decode tuple bytes.
- `SqlTupleCodec` remains in `adapter/`; it is a persistence-format translation component, not Domain logic.
- RDB public Actions remain the only route to database operations.

The exact port method signatures should be finalized in the implementation PR after checking all port implementations and tests. Do not expose tuple bytes through the revised port.

## 3. Responsibility inventory

### A. Keep in ExecuteQueryAction (orchestration/boundary)

| Current symbol/responsibility | Reason to keep |
| --- | --- |
| Constructor and injected references | Store the parser contract and required RDB port. |
| `execute(std::string_view)` | Public entry point: parse input, dispatch the parsed statement, and translate failures into the public outcome. |
| Top-level statement dispatch in `executeStatement` | Coordinate the execution path and invoke the required Domain operations and RDB port calls. |
| RDB calls | These are external I/O and must remain through `RdbServicePort`. |
| Fetching schema/rows before a pure operation | The Action coordinates when external data is loaded; Domain receives the data explicitly. |
| Parse/error/result boundary conversion | Preserve the public `QueryExecutionOutcome` contract and existing error behavior. |
| Execution-specific sequencing/state | Keep the sequence of schema fetch, row fetch, domain operation, and result return in QES orchestration. |

### B. Move or delegate to QES Domain

These operations are pure once their input data is passed explicitly. They must not call services, Actions, ports, adapters, Container, or storage.

| Current symbol/responsibility | Proposed Domain ownership | Notes |
| --- | --- | --- |
| `schemaTypes` and supported SQL schema type validation | `domain/schema/` | Validate SQL schema types without database access. |
| `resolveJoinColumn`, `resolveSelectedJoinColumn`, `resolveWhereJoinColumn` | `domain/binding/` | Pure schema/qualifier resolution. Preserve unknown/ambiguous/invalid-reference errors. |
| `normalizeSingleTableColumnName`, `normalizeSingleTableWhereColumns`, `qualifyJoinWhereColumns` | `domain/binding/` | Pure transformation of parsed references using explicit schema/table/alias context. |
| `compileTextFunction`, `compileIntegerExpression` | `domain/binding/` or `domain/expression/` | These bind parsed expressions to schema indexes; keep evaluation in the existing scalar evaluator. Separate binding from evaluation cleanly. |
| `textFunctionName` and aggregate display-name construction | `domain/metadata/` or focused SQL naming helpers | Pure SQL output naming rules; avoid duplicating name construction in grouped/ungrouped paths. |
| `validateInnerJoinColumns` and JOIN feature-shape validation | `domain/validation/` | Pure validation over the statement and explicit schemas. |
| `executeInnerJoin`, `executeCrossJoin`, `executeMultipleInnerJoins` row-combination algorithms | `domain/relational/` | The Action must fetch/decode rows first; the Domain operation receives typed rows and returns typed joined rows/projection metadata. No I/O inside Domain. |
| UNION ALL branch compatibility and row concatenation | `domain/relational/` | Keep recursive branch execution in the Action; delegate only pure schema compatibility and combination. |
| UNION duplicate elimination | `domain/relational/` | Pure row-set operation; preserve first-occurrence order. |
| GROUP BY key construction and grouping | `domain/aggregate/` | The existing aggregate calculator already owns value reduction; move pure grouping mechanics while leaving fetch and execution sequencing in QES. |
| HAVING comparison evaluation | `domain/aggregate/` or `domain/predicate/` | Pure comparison against aggregate output; preserve supported comparison operators. |
| DISTINCT row elimination | `domain/relational/` | Pure row-set operation; preserve stable first-occurrence semantics. |
| ORDER BY row ordering and grouped ordering | `domain/relational/` | Pure stable ordering over explicit indexes/keys; preserve alias ordering and tie behavior. |
| LIMIT/OFFSET slicing | `domain/relational/` | Pure row-window operation; preserve current bounds behavior. |
| Projection row construction | `domain/relational/` | Delegate pure projection over explicit indexes/compiled expressions; keep result envelope construction in QES. |
| Repeated comparison-operator switches for JOIN/HAVING | `domain/value/` or `domain/predicate/` | Centralize comparison semantics and avoid multiple subtly different implementations. Reuse `domain::compareSqlValues`. |
| `commandResult` / `executionError` | Keep in Action or a tiny result factory only if reused | These create the public QES result/error envelope, not query algorithms. Avoid unnecessary abstractions. |

### C. Reuse existing Domain components

Do not re-extract or duplicate these SIM-79 components:

- `domain/predicate/WherePredicateEvaluator`: WHERE compilation/matching and SQL value comparison.
- `domain/expression/ScalarExpressionEvaluator`: integer arithmetic and scalar TEXT function evaluation.
- `domain/aggregate/SqlAggregateCalculator`: COUNT/MIN/MAX/SUM/AVG value reduction.
- `domain/parser/`: lexer/parser implementation and parser tests.
- `domain/contract/`: SQL parser contract and typed statement/value representations.

New Domain files should have one clear responsibility, explicit inputs/outputs, no hidden state, and focused unit tests.

### D. Keep in Adapter

- `SqlTupleCodec::encode/decode` and little-endian tuple-byte handling remain in `adapter/`.
- `RdbServiceAdapter` remains the implementation of `RdbServicePort` and the only QES component that translates between the QES SQL data representation and RDB tuple-byte representation.
- Do not move tuple-byte encoding into Domain or expose tuple/storage internals through Domain APIs.

## 4. Target Action shape

The final Action should read like an execution recipe, not an implementation of SQL algorithms:

1. Parse SQL through the parser contract.
2. Convert parse failures to the public error result.
3. Dispatch the typed statement.
4. Fetch only the schema/rows needed through `RdbServicePort`.
5. Call focused Domain operations with explicit typed inputs.
6. Call the RDB port for writes where required.
7. Construct and return the public result.
8. Translate execution exceptions at the Action boundary.

The Action must not implement comparison algorithms, name-binding algorithms, join loops, grouping mechanics, row sorting, duplicate elimination, projection algorithms, tuple encoding, or tuple decoding.

The header should expose the existing public result/outcome and Action contract only. Keep private declarations only when they materially improve the interface; do not create a large private method tree that merely relocates the monolith without clear ownership.

## 5. Revised implementation plan — 5 PRs total

The initial three-PR estimate is too compressed for the actual responsibility inventory. The safer estimate is **5 PRs including this audit**, with each implementation PR independently testable.

### PR 1 — Audit ExecuteQueryAction responsibilities and dependencies (this PR)

- [x] Inspect Action header/implementation and current dependencies.
- [x] Inspect the RDB port and adapter boundary.
- [x] Inventory orchestration, I/O, pure query logic, and tuple-format responsibilities.
- [x] Record the proposed Domain ownership and implementation order.
- [ ] User review and approval of this audit.

### PR 2 — Move tuple serialization behind the RDB adapter

- [ ] Change the QES RDB port to exchange typed SQL values/rows rather than tuple byte buffers.
- [ ] Move `SqlTupleCodec` encode/decode calls into `RdbServiceAdapter`.
- [ ] Keep the adapter using only RDB public Actions.
- [ ] Update the Container only if constructor wiring changes.
- [ ] Update port fakes and focused adapter/Action tests.
- [ ] Verify tuple bytes and persisted database compatibility are unchanged.

**Done when:** `ExecuteQueryAction` no longer includes or calls `SqlTupleCodec`, and tuple-byte representation is hidden behind the adapter.

### PR 3 — Extract pure schema binding and statement validation

- [ ] Extract schema type validation and pure statement/schema validation.
- [ ] Extract single-table and multi-table column/alias resolution.
- [ ] Extract WHERE reference normalization/qualification that is independent of I/O.
- [ ] Extract expression-to-schema binding while reusing the existing scalar evaluators.
- [ ] Extract JOIN shape/column validation.
- [ ] Add focused Domain unit tests for valid, unknown, ambiguous, mismatched, and malformed references.
- [ ] Keep schema fetching and execution sequencing in the Action.
- [ ] Preserve existing error messages/observable behavior where practical and required by tests.

**Done when:** The Action no longer implements reusable schema/name-binding or statement-validation algorithms.

### PR 4 — Extract pure relational query operations

- [ ] Extract JOIN row-combination algorithms; pass typed rows and resolved plans explicitly.
- [ ] Extract UNION compatibility/combination and stable duplicate elimination.
- [ ] Extract GROUP BY formation while reusing `SqlAggregateCalculator`.
- [ ] Extract HAVING comparison logic.
- [ ] Extract DISTINCT, ORDER BY, LIMIT/OFFSET, and projection row operations into focused components.
- [ ] Avoid creating one oversized generic query-engine class; group by cohesive relational responsibility.
- [ ] Add focused tests for ordering stability, duplicate handling, join combinations, group keys, pagination boundaries, and projection.
- [ ] Keep all database reads/writes and execution sequencing in QES.

**Done when:** Pure relational algorithms are independently testable and the Action only coordinates data loading and their invocation.

### PR 5 — Final Action simplification and architecture verification

- [ ] Reduce `ExecuteQueryAction.cpp` to parsing/dispatch, required RDB port calls, Domain delegation, and result/error boundary handling.
- [ ] Keep `ExecuteQueryAction.h` concise and free of implementation-detail types.
- [ ] Remove obsolete includes, helper functions, duplicated code, and unnecessary dependencies.
- [ ] Confirm the Action has only the parser contract and required RDB I/O port as constructor dependencies unless the audit-approved design changes.
- [ ] Confirm Domain has no dependency on services, Actions, ports, adapters, Container, or storage.
- [ ] Confirm QES accesses RDB only through public RDB Actions via the existing adapter/port.
- [ ] Run focused Domain, adapter, parser, and QES tests.
- [ ] Run architecture/dependency checks and `./scripts/test.sh`.
- [ ] Review the complete diff and current-head CI results; resolve CodeRabbit/review findings.

**Done when:** The Action is a concise orchestration layer, all pure business logic has a clear Domain owner, and relevant tests/checks pass.

## 6. Non-negotiable architecture rules

- No business-logic algorithms in `ExecuteQueryAction`.
- No tuple-byte encoding/decoding in `ExecuteQueryAction`.
- Only required external I/O through `RdbServicePort`.
- Parser contract remains a pure non-I/O dependency.
- No direct CatalogService/TupleService/storage dependencies.
- Domain receives data explicitly and never performs I/O.
- No duplicated implementation of SIM-79 Domain behavior.
- No SQL syntax, query result, error contract, or persisted tuple-format changes.
- Keep PRs focused; split PR 3 or PR 4 further if a change is too broad.
- Request user review when implementation is ready; do not merge without explicit user approval.

## 7. Validation for this audit PR

- [x] Inspected current Action header and implementation.
- [x] Inspected current RDB port and adapter.
- [x] Inspected existing QES Domain interfaces and SIM-79 ownership audit.
- [ ] Confirm Markdown and document diff.
- [ ] Open PR for user review.

No C++ build/test run is required for this documentation-only PR.
