# PostgreSQL Date/Time Types — Design Proposal

> **Status: PROPOSED — FOR REVIEW; NOT APPROVED FOR IMPLEMENTATION**
>
> This document defines a proposed semantic model for SIM-97. It does not authorize production-code, catalog, or row-format changes. PostgreSQL 18 is the compatibility reference.

## 1. Scope

The target SQL types are:

| SQL type | Canonical identity | Meaning |
|---|---|---|
| `DATE` | `Date` | Calendar date, with no time of day or timezone |
| `TIME(p) WITHOUT TIME ZONE` | `Time` | Local time of day |
| `TIME(p) WITH TIME ZONE` / `TIMETZ` | `TimeWithTimeZone` | Time of day plus its stored UTC offset |
| `TIMESTAMP(p) WITHOUT TIME ZONE` | `Timestamp` | Local calendar date and time, with no timezone |
| `TIMESTAMP(p) WITH TIME ZONE` / `TIMESTAMPTZ` | `TimestampWithTimeZone` | An absolute instant |
| `INTERVAL` | `Interval` | Calendar-relative span with separate month, day, and sub-day components |

The SQL type registry already contains canonical IDs for these types and a temporal precision modifier. This design fills in value, semantic, and persistence decisions; it does not propose adding duplicate identities for aliases.

## 2. Non-negotiable semantic distinctions

1. A `DATE` is not midnight stored as a timestamp.
2. `TIME` is a local clock reading; it is not a duration or an instant.
3. `TIMESTAMP` is a local date/time tuple. It must not be silently interpreted as UTC.
4. `TIMESTAMPTZ` identifies an instant. An explicit input offset is used to resolve that instant; the original input zone name/offset is not retained as the value's identity. Output uses the configured session timezone.
5. `TIMETZ` retains a time-of-day and its offset. It is not interchangeable with `TIMESTAMPTZ`; it has no date with which to apply seasonal timezone rules.
6. `INTERVAL` must not be flattened into seconds. A month is not a fixed number of days, and a day is not always 24 elapsed hours across daylight-saving transitions.

These distinctions are required for correct comparisons, arithmetic, and persistence.

## 3. Proposed runtime representations

Keep the runtime value model independent of the row codec. Use small strongly typed wrappers so the SQL value variant cannot confuse two temporal types that happen to share an underlying integer representation.

| Runtime value | Proposed representation | Invariant |
|---|---|---|
| `SqlDate` | Signed day count relative to 2000-01-01 | Proleptic Gregorian calendar; no time or timezone |
| `SqlTime` | Signed 64-bit microseconds since midnight | Range 00:00:00 through 24:00:00, with 24:00 represented explicitly as the end-of-day boundary |
| `SqlTimeWithTimeZone` | `SqlTime` plus signed offset seconds east of UTC | Preserve the supplied offset; no session-zone conversion at display time |
| `SqlTimestamp` | Day count plus microseconds since local midnight | Local fields only; never UTC-normalize |
| `SqlTimestampWithTimeZone` | Signed 64-bit microseconds from the UTC instant corresponding to 2000-01-01T00:00:00Z | Store an instant; do not retain the original input zone |
| `SqlInterval` | Signed 32-bit months, signed 32-bit days, signed 64-bit microseconds | Keep calendar months, calendar days, and sub-day time separate |

The epoch is an internal representation choice, not a promise about the public SQL epoch literal. Conversion and formatting belong to pure QES domain helpers.

### 3.1 Calendar and range handling

- Use a proleptic Gregorian calendar for finite dates, including supported BC dates.
- Do not use `std::chrono::year` as the sole persisted year representation: its bounded year type cannot represent PostgreSQL's full documented date/timestamp range.
- A day-count representation avoids platform-dependent `time_t` limits.
- Validate month lengths, leap years, year zero / BC conversion, and all supported range boundaries in pure helpers.
- PostgreSQL's special values such as `infinity`, `-infinity`, and `epoch` are not silently encoded as ordinary finite values. Decide their support in a separately scoped compatibility task; until then, reject unsupported special values clearly.

### 3.2 Fractional-second precision

PostgreSQL accepts temporal precision `p` from 0 through 6. The default is microsecond precision.

- Store temporal values at microsecond resolution.
- For declared precision `p`, normalize input to the corresponding decimal-second precision using PostgreSQL-compatible rounding, not arbitrary truncation.
- Keep the declared precision in the canonical type descriptor/catalog metadata.
- Reject precision outside 0..6 at type declaration time.
- `DATE` has no temporal precision modifier.
- For `INTERVAL`, precision applies to seconds; when a field mask is also present, that mask must include `SECOND`.

## 4. Timezone model

### 4.1 Session timezone

Introduce one explicit session-timezone setting owned by the SQL session/configuration layer. It must be passed as context to QES operations that parse timezone-less input for `TIMESTAMPTZ` or render a `TIMESTAMPTZ` result.

- Default the initial UniDB session timezone to UTC for deterministic behavior.
- Support numeric UTC offsets first only if the implementation ticket explicitly scopes that subset.
- Named IANA zones and daylight-saving rules require an IANA timezone database; do not approximate them with a fixed offset.
- Do not let a process-global timezone silently determine SQL results.

### 4.2 Input and output rules

- `TIMESTAMP WITHOUT TIME ZONE`: parse and preserve local date/time fields. If the input contains a timezone indication, PostgreSQL ignores that indication for a value already typed as `TIMESTAMP`; match this only if the accepted literal grammar can distinguish the explicit type.
- `TIMESTAMPTZ`: an explicit offset/zone resolves the instant. If no zone is present, interpret the local fields in the configured session timezone. Store the resulting instant in UTC-based representation.
- `TIMESTAMPTZ` output: convert the stored instant to the current session timezone before formatting.
- `TIMETZ`: preserve the offset as part of the value and display that stored offset. Do not apply the session timezone to it.
- `TIME WITHOUT TIME ZONE`: remains a local clock value. A timezone suffix must not cause an implicit conversion to another clock time.

### 4.3 DST and named zones

Use IANA timezone rules for named zones. The exact behavior for ambiguous and nonexistent local times must be verified against PostgreSQL 18 with differential tests and recorded in the implementation contract. Do not invent a policy or silently choose an offset.

A timezone database/library choice is still open. Before selecting one, verify:
- C++20 toolchain support on the project's supported macOS and Linux environments.
- Availability and update policy of IANA tzdb data.
- Deterministic tests independent of the host machine's local timezone.
- License and packaging implications for the CLI and Docker image.

## 5. INTERVAL model

Keep three independently signed fields:

- `months`: calendar months, including years normalized as 12 months.
- `days`: calendar days, including weeks normalized as 7 days.
- `microseconds`: hours, minutes, seconds, and fractional seconds.

Do not convert months to 30 days or days to 86,400 seconds when storing or comparing an interval in isolation.

### 5.1 Field masks and precision

PostgreSQL supports interval field restrictions such as `YEAR`, `MONTH`, `DAY`, `HOUR`, `MINUTE`, `SECOND`, and compound ranges such as `YEAR TO MONTH` and `DAY TO SECOND`.

The current generic type modifier has `temporalPrecision` but no interval field mask. The type descriptor design must therefore add an explicit interval-field representation (for example, a validated enum for the leading/last field pair), rather than overloading character length or numeric precision.

When a field mask is declared:
- Validate the legal field combinations.
- Apply PostgreSQL's field interpretation and discard/rounding rules deliberately.
- Require `SECOND` in the mask when fractional-second precision is specified.
- Persist the mask and precision with the type descriptor.

### 5.2 Arithmetic

- Adding an interval to a date/timestamp applies months and days as calendar operations, then applies the time component.
- Month-end behavior (for example, adding one month to a date near the end of a long month) must match PostgreSQL.
- Timezone-aware arithmetic across DST boundaries must use the relevant timezone rules; a calendar day is not assumed to equal 24 elapsed hours.
- Interval-to-interval ordering and equality must follow a deliberately specified PostgreSQL-compatible rule. Do not compare by an undocumented flatten-to-microseconds shortcut.

## 6. Type descriptors and aliases

The canonical identities already present in `SqlTypeId` are:

- `Date`
- `Time`
- `TimeWithTimeZone`
- `Timestamp`
- `TimestampWithTimeZone`
- `Interval`

Existing aliases in the registry should resolve to those identities:

- `TIME` and `TIME WITHOUT TIME ZONE` → `Time`
- `TIMETZ` and `TIME WITH TIME ZONE` → `TimeWithTimeZone`
- `TIMESTAMP` and `TIMESTAMP WITHOUT TIME ZONE` → `Timestamp`
- `TIMESTAMPTZ` and `TIMESTAMP WITH TIME ZONE` → `TimestampWithTimeZone`

Required descriptor changes:
- Keep `temporalPrecision` for TIME, TIMETZ, TIMESTAMP, TIMESTAMPTZ, and INTERVAL.
- Reject temporal precision on DATE.
- Add a validated interval field-mask modifier.
- Ensure alias normalization does not erase whether a type is timezone-aware.

Typed literal parsing must respect the explicit type name. Do not infer `TIMESTAMPTZ` merely because a string contains `+02` or `Z`: PostgreSQL determines the literal's type before examining the literal string.

## 7. SQL behavior matrix for the initial supported subset

| Operation | DATE | TIME | TIMETZ | TIMESTAMP | TIMESTAMPTZ | INTERVAL |
|---|---|---|---|---|---|---|
| Equality / ordering | Calendar order | Clock order | PostgreSQL offset-aware value semantics | Local field order | Instant order | PostgreSQL interval semantics |
| INSERT assignment | Validate date | Validate clock/precision | Validate clock and offset | Validate local fields/precision | Resolve to instant using session timezone when needed | Parse fields and normalize |
| Result formatting | ISO date initially | ISO time | Time plus stored offset | ISO local timestamp | Session-zone timestamp with offset | Documented normalized interval form |
| Arithmetic | Date ± integer days; date ± interval if scoped | Time ± interval if scoped | Only explicitly defined PostgreSQL operations | Timestamp ± interval | Timestamptz ± interval using timezone context | Interval arithmetic |

The table is a scope proposal, not a claim that every listed operator is already supported. Each operator must have a dedicated acceptance test before being exposed.

## 8. Physical persistence proposal

The row codec remains owned by QES domain and stores typed values only after schema descriptors have been validated. RDBService receives its own RDB-owned schema/value boundary representation through its public Actions; it must not import QES types. CatalogService persists catalog-owned type IDs and modifiers. TupleService continues to store opaque bytes.

Proposed fixed representations for the first versioned codec that supports temporal values:

| Value | Candidate payload |
|---|---|
| DATE | 32-bit signed day count |
| TIME | 64-bit signed microseconds |
| TIMETZ | 64-bit time + 32-bit offset seconds |
| TIMESTAMP | 64-bit day/time scalar or an explicitly specified day + time pair |
| TIMESTAMPTZ | 64-bit UTC microseconds |
| INTERVAL | 32-bit months + 32-bit days + 64-bit microseconds |

Before implementation, choose and document one exact byte order, range validation, and malformed-row behavior. Do not persist C++ struct memory directly: padding, alignment, and host endianness are not a file format.

The new temporal encoding must be part of the explicitly versioned row format. Coordinate row-version changes with SIM-78; do not create a competing NULL bitmap or row header. Existing database-file compatibility is not required, but unknown versions and truncated values must be rejected.

## 9. Ownership boundaries

| Component | Owns |
|---|---|
| SQLParser | Grammar for type declarations, typed literals, interval field qualifiers, and supported operators |
| QES type folder | SQL temporal type IDs, descriptors, modifiers, and strongly typed runtime value wrappers; no imports from other service folders |
| QES domain | Pure calendar validation, parsing/formatting, precision normalization, timezone conversion context, comparison, and arithmetic |
| QES Actions | Orchestrate SQL execution; no temporal business logic in Actions |
| QES RDB adapter | Translate QES-owned schema/value data to/from the RDB public Action contract |
| RDBService | Its own public schema/value boundary types and data integrity rules; no QES type imports |
| CatalogService | Persist catalog-owned canonical type IDs and modifiers in its versioned catalog representation |
| TupleService | Store opaque tuple bytes; no SQL temporal interpretation |
| Container / session | Inject services and provide explicit session timezone context; no temporal semantics in the composition root |

No QES code may import CatalogService or TupleService internals. No shared cross-service header should be introduced merely to reuse temporal structs.

## 10. Staged implementation plan

1. **Pure value foundation:** wrapper structs, calendar algorithms, strict ISO-oriented parsing/formatting, precision normalization, finite ranges, and interval normalization. Unit tests only; no storage or SQL integration.
2. **Catalog and row codec:** persist canonical identities, precision, and interval field mask; implement explicit versioned encoding/decoding and malformed-data tests.
3. **SQL declarations and literals:** CREATE TABLE aliases/modifiers and explicit typed literals; preserve descriptors across the QES–RDB public boundary.
4. **Assignment and basic query semantics:** INSERT validation, SELECT result formatting, comparisons, WHERE, ORDER BY, DISTINCT, and GROUP BY.
5. **Arithmetic and timezone integration:** date/time/interval operators, session timezone, IANA timezone support, DST behavior, and differential tests.
6. **Compatibility audit:** compare supported SQL and expected errors against PostgreSQL 18; document unsupported syntax and semantics.

Each stage should be its own bounded Linear child ticket/PR. Do not combine all temporal types into one implementation PR.

## 11. Explicitly deferred behavior

Unless a later ticket adds it with PostgreSQL 18 differential tests, the first temporal release does not claim support for:
- `infinity`, `-infinity`, `now`, `today`, `tomorrow`, or `yesterday` special inputs.
- Every PostgreSQL DateStyle input/output variant.
- All timezone abbreviations and every historical IANA rule.
- Every PostgreSQL date/time function, `AT TIME ZONE`, `EXTRACT`, `date_trunc`, or `to_char`.
- Every interval literal spelling and all SQL-standard interval qualifier combinations.
- SQL NULL or three-valued logic (SIM-78).

Unsupported cases must fail explicitly, not silently fall back to strings or integers.

## 12. PostgreSQL 18 reference notes

The design is based on the PostgreSQL 18 documentation:
- [Date/Time Types](https://www.postgresql.org/docs/18/datatype-datetime.html): type distinctions, ranges, precision 0..6, timezone input/output, session timezone, and interval representation.
- [Date/Time Functions and Operators](https://www.postgresql.org/docs/18/functions-datetime.html): arithmetic and function semantics to be scoped by later implementation tickets.
- [Data Types](https://www.postgresql.org/docs/18/datatype.html): canonical names and aliases.

Important verified points:
- PostgreSQL stores timezone-aware timestamps internally as UTC and renders them in the current session `TimeZone`.
- PostgreSQL interval values keep months, days, and microseconds separate because month lengths vary and daylight-saving days may not be 24 elapsed hours.
- `TIME WITH TIME ZONE` is supported but PostgreSQL generally recommends timestamp types for real-world timezone use.
- Precision for TIME, TIMESTAMP, and INTERVAL is 0 through 6 fractional-second digits.

## 13. Approval gate

This document is a proposal. Implementation may begin only after Soujash reviews and approves:
1. The runtime representations and range policy.
2. The session timezone and IANA tzdb strategy.
3. The interval field-mask descriptor design.
4. The physical encoding and row-version coordination with SIM-78.
5. The first bounded implementation slice.

No production type enum, parser, catalog, row codec, or execution changes are included in this design PR.
