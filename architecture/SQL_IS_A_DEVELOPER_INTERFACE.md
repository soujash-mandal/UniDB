# SQL Is a Developer Interface

## Principle

SQL is a language users and developers may choose to interact with UniDB. It is
not the database itself, a required layer of the storage architecture, or the
permanent protocol for applications.

UniDB's core capabilities must remain usable without parsing or executing SQL.
A client should be able to use a supported API or protocol without constructing
SQL strings or depending on SQL syntax and AST types.

## Separate the responsibilities

- **Database core and services** own data, schemas, storage, transactions, and
  reusable database operations. Their public contracts describe database
  capabilities, not SQL grammar.
- **SQL interface** parses SQL and translates it into calls to those existing
  capabilities. It is one possible frontend, primarily useful for people,
  developer tools, and SQL-compatible clients.
- **RPC and application APIs** expose structured, typed requests and responses.
  They must not require SQL strings, SQL AST objects, or the SQL execution
  service to perform database operations.
- **CLI and other developer tools** may offer SQL because it is convenient.
  That convenience must not leak into the database's core contracts.

The current implementation may use QueryExecutionService (QES) to execute SQL.
That is an implementation detail of the current SQL frontend, not a rule that
future APIs, protocols, or database capabilities must depend on QES.

## Shared capabilities, not duplicated implementations

SQL and structured APIs should reach the same underlying database capabilities.
When behavior is useful to more than one interface, place it in the service or
domain that owns that behavior and expose it through that service's public
Actions.

Do not duplicate database behavior in separate SQL and RPC implementations.
Do not move reusable database behavior into a SQL parser or SQL-only layer just
because SQL was the first interface to need it.

## Rules for design and code review

1. Keep SQL strings, tokens, grammar, parser nodes, and SQL-specific types out
   of storage, catalog, tuple, and other core service contracts.
2. Keep RPC/API request and response types independent of SQL syntax and AST.
3. A structured API operation must not need to generate SQL and send it through
   QES internally.
4. SQL may call the public Actions of the services that own the required
   capabilities, following UniDB's service-boundary rules.
5. Put reusable, interface-independent behavior in its owning service/domain;
   keep SQL-specific translation and orchestration in the SQL frontend.
6. Do not add or change a core service capability solely to imitate a SQL
   construct. First determine whether the requested behavior is a reusable
   database capability or only a syntax-level convenience.
7. Treat the current QES architecture as the current SQL implementation, not
   as a required foundation for future RPC/API or other interfaces.

## Example

A SQL client may submit:

```sql
SELECT id FROM people WHERE department_id IN (10, 20, 30);
```

A structured API should express the same intent with typed fields, for example:

```json
{
  "operation": "select",
  "table": "people",
  "columns": ["id"],
  "filter": {
    "column": "department_id",
    "operator": "in",
    "values": [10, 20, 30]
  }
}
```

This JSON is illustrative, not a finalized UniDB API contract. The actual
structured API shape must be designed separately around UniDB's supported
operations and service boundaries.

## Roadmap implication

Build database capabilities first, independently of any one query language.
Then expose those capabilities through whichever interfaces UniDB supports:
SQL for developer convenience, structured RPC/API for applications, and CLI
or other tools for operators and developers.

Adding a SQL feature should not automatically create an RPC/API feature, and
adding an RPC/API feature should not require SQL support. Both interfaces
should reuse the same underlying capabilities where their behavior overlaps.
