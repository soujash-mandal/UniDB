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
point values, boolean values, joins, or subqueries.

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
