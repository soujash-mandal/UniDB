# QES Refactor Audit — SIM-79

**Audit base:** `main` at `5a729da59656d84c85e79b6a763555a21a6eabc3`  
**Audit type:** Read-only architecture and ownership audit. No runtime code or SQL behavior is changed by this document.

## 1. Current structure and findings

- `QueryExecutionService/actions/ExecuteQueryAction.cpp` is 2,267 lines and currently owns parsing hand-off, statement validation, schema binding, predicate compilation/evaluation, expression evaluation, join planning/execution, aggregation, sorting, projection, and result construction.
- Most helper functions and helper structs are inside one anonymous namespace in that file. They are private to the translation unit and have no focused unit-test seam.
- QES currently has no `domain/` directory. Existing services use service-local `domain/` folders, so the proposed destination is `QueryExecutionService/domain/` for QES-owned SQL/query semantics. Do not create a repository-wide generic domain folder.
- The current QES tests are primarily end-to-end action tests under `QueryExecutionService/test/ExecuteQueryAction/`. There are codec-specific tests under `QueryExecutionService/test/SqlTupleCodec/`.
- SQL parser implementation, lexer, and parser contract are currently root-level modules: `SQLParser/` and `SQLParserContract/`. The contract is included by QES Action, QES port, and the tuple codec. Parser implementation is constructed by `container/container.h` and referenced by QES and CLI tests.
- `CMakeLists.txt` lists parser sources and parser tests explicitly. The Docker persistence workflow also has path filters for both current parser directories.

## 2. Method ownership map

The line numbers below refer to the audit base SHA above. They are navigation aids; use symbol names when applying the plan.

### A. Good candidates for QES domain extraction

These functions implement SQL value/expression rules and can be made deterministic, stateless domain operations. They should receive their inputs explicitly and must not call RDB Actions, ports, adapters, or storage.

| Current symbol(s) | Current location | Proposed destination | Reason / boundary |
| --- | --- | --- | --- |
| `compareSqlValues` (around line 54) | `ExecuteQueryAction.cpp` anonymous namespace | `domain/value/SqlValueComparison` | Typed INT/TEXT comparison is a reusable SQL value rule. Preserve current type-mismatch behavior. |
| `matchesLikePattern` (around line 68) | same | `domain/predicate/LikePatternMatcher` | Pure implementation of the existing byte-based `%` and `_` rules. Preserve case sensitivity and no-escape behavior. |
| `matchesWhere` (around line 97) | same | `domain/predicate/WherePredicateEvaluator` | Evaluates one already-bound predicate against a value. Keep the current LIKE, IN, BETWEEN, and comparison semantics. |
| `CompiledWhereExpression`, `compileWhereExpression`, `matchesWhereExpression` (around lines 158–258) | same | `domain/predicate/WhereExpression` | The compiled expression representation and recursive boolean evaluation are deterministic. Keep schema/column binding explicit; do not give this code database access. If binding and evaluation become clearer as separate types, split them rather than creating a generic framework. |
| `evaluateTextFunction` and its compiled input type (around lines 498–549) | same | `domain/expression/SqlTextFunctionEvaluator` | UPPER/LOWER/LENGTH are pure value transformations. Preserve ASCII-only case conversion and byte-count LENGTH. |
| `evaluateIntegerExpression` and its compiled input type (around lines 563–678) | same | `domain/expression/IntegerExpressionEvaluator` | Arithmetic is deterministic and stateless. Preserve overflow, division-by-zero, and INT64_MIN edge-case errors. |
| Aggregate value reduction currently duplicated in grouped and ungrouped branches (around lines 1949–1988 and 2076–2117) | `executeStatement` | `domain/aggregate/SqlAggregateCalculator` | COUNT/MIN/MAX/SUM/AVG value calculation is pure over an explicit set of typed values. Extract the reducer only; keep fetching rows, forming groups, HAVING, output ordering, and pagination in QES. Preserve empty-input and overflow semantics. |

**Important split inside the candidate set:** schema binding/validation helpers such as `compileIntegerExpression` and `compileTextFunction` resolve names against a table schema. They are pure today, but they are query-planning responsibilities rather than value evaluation. Keep them in QES initially unless the implementation can cleanly separate a reusable domain rule from QES-specific column binding. Do not move a whole function merely because it has no I/O.

### B. Keep in QES orchestration / query planning

| Current symbol(s) | Why it stays in QES |
| --- | --- |
| `ExecuteQueryAction::execute`, `executeStatement` | Public action boundary, parsing hand-off, exception-to-result mapping, statement dispatch, and orchestration. |
| `schemaTypes` | Validates the schema returned by the RDB port for the current V1 execution path. It is part of QES boundary validation. |
| `resolveJoinColumn`, `resolveSelectedJoinColumn`, `resolveWhereJoinColumn`, `validateInnerJoinColumns` | Resolve parsed query references against one or more fetched table schemas and enforce the current JOIN feature restrictions. Keep this in QES query planning for now. |
| `normalizeSingleTableColumnName`, `normalizeSingleTableWhereColumns`, `qualifyJoinWhereColumns` | Rewrite parsed names using the current table/alias context. These are query-context transformations, not general SQL value operations. |
| `compileTextFunction`, `compileIntegerExpression` | Bind parsed column references to schema indexes. Keep the schema lookup and query-context validation in QES; only extract the pure evaluators. |
| `executeInnerJoin`, `executeCrossJoin`, `executeMultipleInnerJoins` | Read rows through `RdbServicePort`, build intermediate rows, and orchestrate query execution. These are not domain methods. |
| `commandResult`, `executionError` | Construct the QES public result/error model. |
| SELECT projection setup, ORDER BY index resolution, DISTINCT application, LIMIT/OFFSET, UNION branch execution, group construction, HAVING, and result assembly | These are interdependent stages of the execution pipeline. Keep them in QES until a separately scoped audit proves a small, cohesive pure reducer can be extracted. |

### C. Keep in the adapter layer, despite being pure

| Current symbol(s) | Why it stays |
| --- | --- |
| `SqlTupleCodec::encode`, `SqlTupleCodec::decode`, `appendLittleEndian`, `readLittleEndian` | These functions are deterministic, but their responsibility is translating SQL values to/from the persisted tuple byte format. That is an adapter/format-boundary responsibility, not SQL business/domain logic. Keep the codec in `adapter/` and retain its focused tests. |

## 3. SQLParser and SQLParser Contract ownership / move map

Move both modules under QES, but preserve two separate responsibilities and use two small PRs because the contract has several direct QES includes while the parser implementation has separate container/test/build references.

### Proposed final layout

- `QueryExecutionService/contract/sqlParser.h`
- `QueryExecutionService/contract/sqlStatement.h`
- `QueryExecutionService/domain/parser/SqlLexer.h`
- `QueryExecutionService/domain/parser/SqlLexer.cpp`
- `QueryExecutionService/domain/parser/SqlParserImpl.h`
- `QueryExecutionService/domain/parser/SqlParserImpl.cpp`
- `QueryExecutionService/domain/parser/test/...` (all current SQLParser tests)

The contract remains declarations and typed statement/value representations only. The parser/lexer remains the implementation that turns SQL text into that contract. Neither layer executes queries or accesses database services.

### Known references to update

- `QueryExecutionService/actions/ExecuteQueryAction.h` includes the parser contract.
- `QueryExecutionService/port/RdbServicePort.h` includes statement contract types.
- `QueryExecutionService/adapter/SqlTupleCodec.h` includes SQL value/type definitions.
- `SQLParser/SqlParserImpl.h` includes the contract.
- `container/container.h` constructs `SqlParserImpl`.
- QES test support and at least `ResolvesInnerJoinColumnsTest.cpp` / `ExecutesInnerJoinTest.cpp` include the parser implementation directly.
- CLI tests construct `SqlParserImpl` through QES test support or direct includes.
- `CMakeLists.txt` lists parser source and test files explicitly.
- `.github/workflows/docker-persistence.yml` currently filters changes under `SQLParser/**` and `SQLParserContract/**`; update these filters when the directories move.

Before the move, run repository-wide searches for `SQLParser/`, `SQLParserContract/`, `SqlParserImpl.h`, `SqlLexer.h`, `sqlParser.h`, and `sqlStatement.h` so no include, workflow path, test, or document is missed.

## 4. Recommended PR sequence after this audit

The previous six-PR estimate should be treated as a minimum planning shape, not a fixed count. This audit shows that the parser contract and parser implementation should be split, and the pure logic has at least three distinct ownership groups.

1. **PR 1 — this audit document.** Documentation only; no behavior changes. User reviews and approves the ownership map.
2. **PR 2 — move SQLParser Contract into QES.** Move the two contract headers to `QueryExecutionService/contract/`; update all includes and build/test references. No parser implementation move in this PR.
3. **PR 3 — move SQLParser implementation into QES.** Move lexer, parser implementation, and parser tests to `QueryExecutionService/domain/parser/`; update Container, tests, CMake, workflow path filters, and docs.
4. **PR 4 — extract SQL value and WHERE predicate semantics.** Move typed value comparison, LIKE matching, predicate evaluation, and recursive WHERE expression evaluation into focused QES domain files. Keep schema/alias binding in QES unless cleanly separable.
5. **PR 5 — extract scalar expression evaluators.** Move integer arithmetic and TEXT function evaluation into focused domain files. Keep schema binding in QES.
6. **PR 6 — extract aggregate reducers.** Extract only COUNT/MIN/MAX/SUM/AVG value calculation; leave grouping, row fetching, HAVING, ordering, and pagination in QES.
7. **PR 7 — final QES cleanup and architecture verification.** Remove obsolete helpers/includes, verify ownership boundaries, and run full regression/architecture checks.

Split any extraction PR further if it mixes unrelated domain responsibilities or becomes difficult to review. Conversely, if an audited candidate does not justify a clean extraction, leave it in QES and record why rather than forcing a PR.

## 5. Architecture invariants for implementation PRs

- QES domain code must be deterministic/stateless and independent of services, Actions, ports, adapters, containers, and storage.
- Domain functions receive data explicitly; no hidden global state, I/O, or service lookups.
- QES continues to access RDB only through the existing public Action-backed port.
- No direct CatalogService, TupleService, BufferPoolManager, DiskManager, or storage-domain dependencies in QES.
- Parser contract types remain independent of services and storage; parser implementation depends on its contract, not on execution services.
- Keep tuple byte encoding in the QES adapter. Do not leak tuple/storage types into QES domain APIs.
- Preserve all current SQL syntax, result values, error semantics, ordering, and persistence format. This refactor is not a feature change.

## 6. Validation plan

For the documentation-only audit PR:
- [ ] Confirm the document matches the current `main` tree and the recorded audit SHA.
- [ ] Check Markdown formatting and links.
- [ ] No C++ build/test run is required because no source, build, or runtime files change.

For implementation PRs:
- [ ] Add focused unit tests for every extracted domain operation, including edge cases and error behavior.
- [ ] Keep or add QES integration tests proving end-to-end query behavior is unchanged.
- [ ] Run the relevant parser, codec, and QES tests for each move/extraction.
- [ ] Run `./scripts/test.sh` and the architecture/dependency checks before the final cleanup PR is marked ready.
- [ ] Inspect the complete diff and current-head CI results; resolve CodeRabbit findings before calling the PR ready.


## 7. Parser placement decision

The final ownership decision places the SQL parser implementation inside QES domain:

- `QueryExecutionService/domain/parser/` owns the lexer, parser implementation, and parser unit tests.
- `QueryExecutionService/contract/` remains separate and owns the parser interface and typed SQL statement/value representations.
- Parser code remains infrastructure-independent: it may depend on the parser contract and standard C++ only; it must not access database services, Actions, ports, adapters, the Container, or storage.
- The parser belongs under `domain/` as an infrastructure-independent part of QES, while retaining a distinct `parser/` subfolder because syntax analysis is a separate responsibility from predicate, expression, and aggregate semantics.
