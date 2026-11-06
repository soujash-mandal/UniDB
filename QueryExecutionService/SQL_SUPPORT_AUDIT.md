# SQL Support Audit: Existing RDBService Action Surface

This document records the PR 0 audit for SIM-74. It answers which proposed SQL features
can be implemented in QueryExecutionService (QES) using the existing RDBService public
Actions, without changing RDBService.

## Scope and constraints

- QES may call only the RDBService public Action layer through its existing adapter/port.
- QES must not depend directly on CatalogService, TupleService, SpaceMapService, or their
  internal domain types.
- Do not add or change RDBService Actions, Ports, Adapters, or service behavior as part of
  the SQL feature work covered by SIM-74.
- This is an implementation-feasibility audit, not a promise that every feature is
  performant for large tables.

## Existing public Action surface available to QES

The current `RdbServiceAdapter` delegates to these RDBService Actions:

| QES operation | RDBService public Action | Data available to QES |
| --- | --- | --- |
| Create a table | `CreateTableAction` | Accepts a SQL table definition and returns the created table ID |
| Read a table schema | `GetTableDefinitionAction` | Returns ordered column names and RDB-owned column types |
| Insert one row | `InsertRowAction` | Accepts one encoded row payload |
| Read all rows | `SelectAllRowsAction` | Returns all live row payloads as `vector<vector<char>>` |

The QES adapter translates the RDB-owned schema into SQL column definitions. QES already
has `SqlTupleCodec` to decode each row payload using the schema and obtain typed
`SqlValue` values (`int64_t` or `string`). The public boundary therefore gives QES
both the ordered schema and the row values required for SQL-level projection and
predicate evaluation.

The row payload is an opaque byte vector at the RDB boundary. QES decodes it using its
own codec; it does not need TupleService types or direct TupleService access.

## Candidate feature audit

| Feature | Existing support in parser / QES | Can be implemented without RDBService changes? | Finding |
| --- | --- | --- | --- |
| SELECT specific columns | Parser AST already stores `columnNames`; QES execution currently rejects anything except `SELECT *` | Yes | Implement projection in QES after decoding rows. Validate requested column names against the schema and preserve requested column order. |
| WHERE equality (`=`) | Parser already produces `WhereEquals`; QES currently rejects every non-empty `where` value | Yes | Implement typed equality comparison in QES using decoded values. This is the smallest WHERE step and should precede other operators. |
| WHERE comparisons (`!=`, `>`, `<`, `>=`, `<=`) | AST has only `WhereEquals`; lexer/parser currently recognize only `=` for predicates | Yes, with parser-contract changes | Extend SQL parser tokens/AST and evaluate comparisons in QES. Define type compatibility and reject unsupported comparisons clearly. No RDBService change is indicated. |
| AND / OR | AST represents at most one equality predicate; parser has no boolean-expression representation | Yes, with parser-contract changes | Add a predicate expression representation and evaluate it in QES. Define precedence and parentheses behavior in the parser before implementation. |
| ORDER BY | No order-by field in the SELECT AST; QES receives all decoded rows | Yes, with parser-contract changes | Parse ordering terms and sort decoded rows in QES using schema column indexes and typed comparisons. This is an in-memory sort, not an index-backed operation. |
| LIMIT / OFFSET | No limit/offset fields in the SELECT AST; QES receives all decoded rows | Yes, with parser-contract changes | Parse bounds and slice the result in QES. The current Action still reads all rows before QES applies the bounds. |
| Multi-row INSERT | Parser AST stores one `values` vector, and `InsertRowAction` inserts one row per call | Yes, with parser-contract changes and an explicit partial-failure contract | Parse multiple value groups and reuse the existing single-row encode/insert path for each row. Inserts are sequential and are not atomic: if a later row fails, earlier rows may already be committed. Do not describe this as an all-or-nothing bulk insert. |

## Important implementation constraints

1. **SELECT work is currently a full scan.** `SelectAllRowsAction` returns every live row
   payload for the table. Projection, filtering, sorting, and limiting can be performed
   in QES, but they do not reduce storage reads or provide index-based execution.
2. **Schema order is authoritative.** Decode row payloads using the ordered schema returned
   by `GetTableDefinitionAction`. Resolve requested columns to schema indexes; do not infer
   positions from user input.
3. **Comparisons must be typed.** V1 SQL values are `int64_t` and `string`. Do not compare
   unlike types or silently coerce them. Define string ordering semantics in tests if
   ordering or range comparisons are implemented.
4. **Multi-row INSERT is not transactional.** The current public API inserts one row at a
   time and exposes no transaction or rollback Action. Report how many rows were inserted
   if a later row fails, or otherwise return an error that makes the partial-commit behavior
   explicit. Do not add a new RDBService bulk Action under this ticket.
5. **No tuple/storage types should leak into QES.** Keep the current RDBService public
   Action boundary and QES-owned SQL value/codec types.

## Recommended implementation order

1. SELECT column projection.
2. WHERE equality.
3. Additional comparison operators.
4. AND / OR predicate expressions.
5. ORDER BY.
6. LIMIT / OFFSET.
7. Multi-row INSERT, after agreeing on partial-failure reporting.

The order above separates the already represented parser features (projection and equality)
from features that first need parser-contract expansion. ORDER BY and LIMIT/OFFSET can be
implemented independently after the parser AST is extended. Multi-row INSERT has a distinct
partial-commit concern and should remain its own focused change.

## Audit conclusion

The existing RDBService public Actions provide enough information and operations to
implement the listed SQL features in QES without changing RDBService. The principal work
is in the SQL parser contract/implementation and QES execution logic. The exception is not
an architectural blocker but an API limitation: multi-row INSERT cannot be atomic with the
current one-row Action. Keep that limitation explicit and out of scope for transaction
semantics.

No RDBService code changes are required for PRs 1–6 in SIM-74 as currently scoped.

## Scalar text functions (SIM-77 PR 7)

QES can evaluate `UPPER`, `LOWER`, and `LENGTH` after reading rows through the
existing public RDB SELECT action. The initial implementation accepts a single
TEXT column reference per function. UPPER/LOWER use ASCII-only case conversion;
LENGTH counts bytes. Results are computed in memory, with no RDBService or
persisted-value changes. JOIN and DISTINCT combinations remain unsupported in
this first increment.


## ORDER BY projection aliases (SIM-77 PR 8a)

ORDER BY can resolve a SELECT output name before resolving a source-column name. This includes explicit aliases and default output names for computed projections. QES evaluates each projection once per filtered row, then sorts using the projected value for alias terms and the decoded source value for ordinary column terms. Multiple terms retain left-to-right priority and stable ordering. This remains an in-memory operation over rows read through the existing public RDB SELECT Action; no RDBService or storage changes are involved. Direct expression syntax inside ORDER BY is not included in this alias-focused increment.


## Grouped aggregate ORDER BY / LIMIT / OFFSET (SIM-77 follow-up)

Grouped aggregate queries can order by projected grouping columns or the aggregate output alias. Multiple terms support ASC/DESC and stable tie ordering. LIMIT and OFFSET are applied after HAVING and sorting. DISTINCT remains unsupported. The implementation operates only on QES result rows produced from the existing RDB public SELECT Action; no storage or RDBService changes are required.
