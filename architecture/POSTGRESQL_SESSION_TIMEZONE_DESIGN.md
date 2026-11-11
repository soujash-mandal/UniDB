# Proposed PostgreSQL-Compatible Session Time Zone Design

**Status:** Proposal — requires owner approval before implementation  
**Linear:** SIM-111  
**Reference behavior:** PostgreSQL 18

## 1. Goals and non-goals

UniDB must distinguish an absolute `TIMESTAMPTZ` instant from the session's civil-time presentation of that instant. Calendar arithmetic must use the session time zone; elapsed-time arithmetic must operate on the instant.

This proposal covers the semantic contract and implementation direction only. It does not implement `SET TIME ZONE`, general session settings, timestamp arithmetic, parsing, storage, or protocol changes.

## 2. Proposed V1 behavior

### Session time zone

- Default each new session to **UTC**. This is deterministic across hosts and containers. It is an intentional compatibility difference from PostgreSQL installations whose configured default is their server-local zone.
- Support the IANA time-zone database names (for example, `America/New_York`, `Asia/Kolkata`) and UTC.
- Support fixed numeric offsets in ISO-8601 form (`+05:30`, `-04:00`) as fixed-offset zones with no DST.
- Reject abbreviations (`PST`, `IST`) and POSIX rule strings in V1. Their meaning can be ambiguous or use a different sign convention.
- Validate a configured zone when the session is created; unknown or malformed values produce a clear invalid-time-zone error. Never silently fall back to UTC.
- The session owns an immutable resolved zone handle/value. Changing a session's zone later must replace that setting atomically; an expression must not observe a partially changed zone.

### Configuration surface

- The server's configured default is UTC; a future startup option/config field may override it.
- The eventual SQL surface is PostgreSQL's `SET TIME ZONE 'zone-name-or-offset'` and `SHOW TIME ZONE`. These commands are explicitly **not** implemented by SIM-111.
- Until SQL session settings exist, tests and internal callers pass an explicit time-zone value. Do not read the host's implicit local zone inside QES domain functions.

### Time-zone database

**Proposal: use a pinned IANA tzdb release distributed with UniDB, accessed through a small timezone adapter.**

- Pin and record the IANA release used by each UniDB build/release.
- Do not download timezone data at runtime.
- Do not silently substitute the host's tzdb. This keeps Docker, macOS, Linux, and CI results reproducible.
- A tzdb update is an intentional dependency/data update, reviewed with regression tests. Existing stored `TIMESTAMPTZ` values remain instants and are not rewritten; future conversions and display use the active release.
- Expose the bundled tzdb version in diagnostics in a later operational/session-settings task.

## 3. Alternatives considered

| Option | Advantages | Risks / decision |
|---|---|---|
| Host OS tzdb | Small deployment; receives OS security/rule updates | Different hosts can produce different results; versions may be unknown; historical coverage differs. Not selected as the semantic source. |
| C++20 `std::chrono::tzdb` | Standard API and good value types | Availability/quality is still library-implementation-dependent; libc++ documents timezone support as experimental. Not safe as the only cross-platform contract. |
| Third-party timezone library using OS data | Mature conversion APIs; can reduce custom code | Still inherits host-version drift and OS-specific behavior. Not selected with OS data mode. |
| Third-party timezone library + pinned IANA data | Portable adapter, deterministic rule set, explicit upgrades | Adds dependency and bundled data maintenance/size. **Preferred direction**, subject to a small prototype validating macOS/Linux builds and packaging. |
| Reuse PostgreSQL's timezone implementation | Closest behavioral reference | Pulls a substantial PostgreSQL-specific subsystem into UniDB and increases maintenance/coupling. Not selected. |

The prototype should compare a maintained IANA-aware library (including Howard Hinnant's `date/tz`) against ICU before choosing the exact dependency. Do not add either dependency until the prototype and build impact are reviewed.

## 4. Canonical conversion and DST rules

- Keep `TIMESTAMPTZ` as signed microseconds from the existing PostgreSQL epoch (2000-01-01 00:00:00 UTC). The zone is session context, never part of the stored value.
- To perform calendar operations, convert the instant to local civil date/time using the session zone's offset/rules.
- When a resulting local civil time is converted back to an instant:
  - For a nonexistent time in a spring-forward gap, use the pre-transition offset (PostgreSQL's standard-time preference).
  - For an ambiguous time in a fall-back fold, use the post-transition offset (again PostgreSQL's standard-time preference).
- Fixed-offset zones have no gaps or folds.
- These rules must be implemented explicitly in the adapter contract, not left to whichever default a library happens to choose.

## 5. TIMESTAMPTZ arithmetic order

Match PostgreSQL's `timestamptz_pl_interval` behavior:

1. Convert the instant to local civil fields in the session zone.
2. Apply interval months to the local calendar month; clamp the day to the last valid day of the target month.
3. Apply interval days to the local calendar date while preserving the local time-of-day.
4. Resolve that local date/time back to an instant using the DST rules above.
5. Apply the interval microseconds directly to the resulting instant (elapsed-time arithmetic), with checked overflow and PostgreSQL finite-range validation.

Subtraction is addition of the negated interval, with checked negation/overflow. Do not collapse interval months, days, and microseconds into one duration.

## 6. Comparison and display

- Compare `TIMESTAMPTZ` values by their canonical UTC instant only. Session TimeZone does not affect equality or ordering.
- Render values in the active session zone, including the correct numeric UTC offset for that instant.
- Changing the session zone changes display, not the stored instant or comparison result.

## 7. Architecture boundaries

- Put reusable zone identity/conversion abstractions in QES domain types (or a small dedicated Foundation timezone service only if multiple services later need it).
- Keep pure calendar/arithmetic rules in QES domain code. Pass the session zone explicitly; do not read process-global timezone state.
- If an external library is selected, isolate it behind a narrow adapter so QES arithmetic does not depend on library-specific types.
- Keep QES connected to RDB only through the existing public Action-backed adapter. No CatalogService/TupleService imports or storage details.
- Container/configuration owns construction and injection of the timezone database/adapter. Avoid introducing a new service until a second real consumer exists.

## 8. Differential test plan

Run the same SQL expressions against PostgreSQL 18 and UniDB for:

- UTC and fixed offsets (`+05:30`, `-04:00`).
- Named zones: `America/New_York`, `Europe/Berlin`, `Asia/Kolkata`.
- Spring-forward gap and fall-back fold resolution.
- Adding one calendar day versus adding 24 hours across both DST transitions.
- Month-end clamping (Jan 31 + 1 month), leap-day arithmetic, negative intervals.
- Offset changes for historical and future dates covered by the pinned tzdb.
- Minimum/maximum finite timestamps and overflow at every arithmetic stage.
- Same instant created from different explicit offsets compares equal and renders differently under different session zones.
- Invalid zone names, rejected abbreviations, malformed numeric offsets, and unavailable tzdb data.

Every test fixture should record the pinned tzdb release and the PostgreSQL 18 expected result.

## 9. Proposed implementation sequence (after approval)

1. Prototype and select the timezone library/data packaging strategy; verify macOS and Linux builds.
2. Add an explicit session TimeZone value/configuration contract and immutable resolved-zone handle.
3. Add instant-to-local and local-to-instant conversion helpers, including PostgreSQL DST gap/fold rules.
4. Implement pure `TIMESTAMPTZ +/- INTERVAL` arithmetic with month/day/microsecond ordering and bounds checks.
5. Add comparison/display integration and PostgreSQL 18 differential tests.
6. Add SQL session-setting commands (`SET TIME ZONE`, `SHOW TIME ZONE`) in a separately scoped SQL-gate task.

**Approval gate:** This is a proposal, not an implementation authorization. Soujash must approve the timezone behavior and dependency/data strategy before source code or dependencies change.

## References

- PostgreSQL 18, Date/Time Types and Time Zones: https://www.postgresql.org/docs/18/datatype-datetime.html
- PostgreSQL source, `timestamptz_pl_interval`: https://github.com/postgres/postgres/blob/REL_18_STABLE/src/backend/utils/adt/timestamp.c
- IANA Time Zone Database: https://www.iana.org/time-zones
- C++ library timezone support notes: https://libcxx.llvm.org/DesignDocs/TimeZone.html
- Howard Hinnant `date/tz` documentation: https://howardhinnant.github.io/date/tz.html
