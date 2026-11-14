The SQL type system has expanded beyond the original INT/TEXT subset. The
currently integrated scalar type families include:

- Integers: SMALLINT, INTEGER (including INT), and BIGINT.
- BOOLEAN.
- Floating point: REAL and DOUBLE PRECISION.
- Exact decimal: NUMERIC and DECIMAL.
- Character strings: CHAR(n), CHARACTER(n), BPCHAR, VARCHAR(n),
  CHARACTER VARYING(n), and TEXT.
- Date/time groundwork: DATE, TIME, TIMETZ, TIMESTAMP, TIMESTAMPTZ, and
  INTERVAL.

Character types use UTF-8 validation and Unicode code-point counting for
length limits. CHAR values are blank-padded and compare without significance
for trailing spaces; VARCHAR and TEXT preserve trailing spaces as significant.
Bounded character types enforce their declared length, including PostgreSQL's
exception for excess trailing spaces.

The date/time type family has value, validation, and arithmetic support, but
its persisted row-encoding integration and complete session-timezone behavior
are still tracked separately. BYTEA, UUID, JSON, and JSONB may be recognized
by the type registry; registry recognition alone does not mean their complete
CREATE/INSERT/SELECT storage and execution behavior is supported.

NULL values are represented by the SQL value model and nullable row storage.
The supported SQL grammar and execution combinations remain deliberately
limited; see the sections below for the currently supported statement and
expression forms.