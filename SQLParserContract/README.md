# SQL Parser Contract

This directory defines the public contract between SQL parsing and query execution.
It contains declarations and documentation only; parser implementation belongs in a
separate implementation change.

## Files

- `sqlStatement.h`: typed representations of supported statements and values.
- `sqlParser.h`: parser interface and success/error result types.

## V1 supported SQL

Keywords are case-insensitive. Identifiers are represented as strings. A trailing
semicolon is optional.

- `CREATE TABLE table_name (column_name INT, column_name TEXT)`
- `INSERT INTO table_name (column_name, ...) VALUES (literal, ...), (literal, ...)`
- `SELECT * FROM table_name`
- `SELECT column_name, ... FROM table_name`
- `SELECT DISTINCT column_name, ... FROM table_name`
- `SELECT column_name AS alias, ... FROM table_name`
- `SELECT DISTINCT * FROM table_name`
- Either SELECT form may include a WHERE expression with comparison operators,
  IN / NOT IN, BETWEEN / NOT BETWEEN, and LIKE / NOT LIKE.
- WHERE expressions support parentheses, AND, and OR (AND has higher
  precedence than OR).

Supported column types are INT and TEXT. Supported literals are signed
integer values and single-quoted text values. V1 does not define NULL, floating
point values, boolean values, or subqueries. A single equality-based INNER JOIN
is supported. Table aliases may be written with or without AS in FROM and INNER
JOIN clauses; when an alias is present, qualified references use that alias.

The parser describes statement syntax; table existence, column existence, type
compatibility, and other execution-time validation belong to query execution and
database services.

## Result and errors

- Success returns a `SqlStatement` containing a `CreateTableStatement`,
  `InsertStatement`, or `SelectStatement`.
- Invalid SQL syntax returns `SqlParseErrorCode::SyntaxError`.
- A valid statement form outside the V1 supported subset returns
  `SqlParseErrorCode::UnsupportedStatement`.

The parser contract has no dependency on UniDB storage services, the container,
buffer pool, disk manager, or query execution.

## LIKE pattern scope

The optional SELECT WHERE clause supports `LIKE` and `NOT LIKE` on TEXT
columns. In a pattern, `%` matches zero or more bytes and `_` matches exactly
one byte. Matching is case-sensitive. There is no ESCAPE clause: backslash has
no special pattern-escaping meaning, and literal percent/underscore escaping
is not supported. LIKE patterns must be single-quoted TEXT literals.

## DISTINCT and column aliases

`DISTINCT` removes duplicate rows from the selected projection: two source rows
are duplicates only when every selected value is equal. It is applied after
WHERE filtering and before ORDER BY, OFFSET, and LIMIT. For a DISTINCT query,
each ORDER BY column must also appear in the SELECT list. Duplicate output
column names (including duplicate aliases or alias/name collisions) are rejected.
Aliases are introduced with the explicit `AS` keyword and are returned as the
result column names; the underlying column types and values are unchanged.

## Aggregate functions without GROUP BY

A SELECT list may contain one aggregate expression: `COUNT(*)`,
`COUNT(column)`, `MIN(column)`, `MAX(column)`, `SUM(column)`, or
`AVG(column)`. An explicit `AS` alias is supported. Aggregate queries may
include WHERE, but cannot be combined with DISTINCT, ORDER BY, LIMIT, or OFFSET.
GROUP BY and HAVING are not part of this change.

COUNT returns an INT count (zero for an empty filtered result). MIN and MAX
support INT and TEXT columns. SUM and AVG require INT columns. AVG returns an
INT using truncating integer division because the V1 value model has no
fractional numeric type. MIN, MAX, SUM, and AVG return an execution error for
an empty filtered result; SUM reports an error if its result exceeds INT64.
NULL values are not supported, so COUNT(column) counts every matched row.
Aggregate evaluation scans rows returned by the existing public RDB SELECT
action; it does not use indexes.

## GROUP BY and HAVING

Grouped queries support one or more grouping columns and one aggregate expression
in the SELECT list, for example:
`SELECT department, COUNT(*) FROM employees GROUP BY department`.
Every selected non-aggregate column must appear in GROUP BY, and the selected
grouping columns must match the GROUP BY list in the same order. Supported
aggregates are COUNT, MIN, MAX, SUM, and AVG with the same type and empty-group
semantics as ungrouped aggregates.

A single HAVING condition may compare the selected aggregate expression with an
INT or TEXT literal using =, !=, >, <, >=, or <=. The HAVING aggregate must match
the SELECT aggregate. WHERE is applied before grouping; HAVING is applied after
each group is aggregated. DISTINCT, ORDER BY, LIMIT, and OFFSET are not
supported with grouped queries. Results are returned in grouping-key order;
this implementation uses an in-memory full scan and grouping map.


## INNER JOIN parser groundwork

The parser recognizes a single equality-based INNER JOIN. Table aliases are
optional and may use either form:

```sql
SELECT u.id, o.amount
FROM users AS u
INNER JOIN orders o ON u.id = o.user_id
```

Aliases may also be used without a JOIN, for example `SELECT u.id FROM users u`.
The AST preserves each underlying table name, optional alias, and qualified
ON-column references.

The next validation stage resolves both table schemas through the
existing RDB public Action-backed port. The ON clause must reference one column
from each table, and both columns must have the same type. SELECT-list references
may be qualified; unqualified references are accepted only when the column name
exists in exactly one table. Output names remain unqualified when unique and
become `table.column` when the name exists in both tables. An explicit `AS`
alias overrides the generated name. Duplicate output names are rejected.

The execution stage currently supports `SELECT *` without a WHERE clause.
It fetches both tables through the existing public RDB SELECT Action-backed port,
decodes the rows, and performs an in-memory nested-loop equality join. Matching
rows are returned with columns from the FROM table followed by columns from the
joined table. A duplicate column name is qualified with its source table name.
The join performs O(N × M) key comparisons for N left rows and M right rows and
holds both decoded inputs plus the output rows in memory.

Explicit projections are supported for qualified columns and unambiguous
unqualified columns. `AS` projection aliases override generated output names.
When a table alias is declared, the alias is the qualifier used for SELECT,
JOIN ON, and WHERE references; the original table name is not a valid qualifier
for that table in the same query. Unqualified references are accepted only
when the column exists in exactly one input table. Ambiguous or unknown
references are rejected before row reads. Self-joins are supported when each
instance has a distinct alias. Filtering and projection run in memory after the
two input tables are fetched. LEFT JOIN and non-equality join conditions remain
unsupported.
