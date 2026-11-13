# SQL Is a Developer Interface, Not the Database Core

## Status

**Architecture principle — approved direction.** This document records the intended long-term boundary for UniDB's interfaces. It does not claim that the future RPC/API implementation already exists.

## Core principle

SQL is a developer-facing language for expressing database operations. It is not the database's internal data model, its permanent application protocol, or a requirement for every client.

UniDB must remain a database engine that can be used without SQL. A developer may choose SQL through the CLI or another SQL client, while an application may use a structured RPC/API contract directly.

The SQL interface must be replaceable or removable without changing the meaning of stored data or breaking clients that use the structured API.

## Separate the responsibilities

| Part | Owns |
|---|---|
| Database core and services | Database capabilities, data integrity, schema and metadata ownership, storage, and service-level operations |
| SQL developer interface | SQL text, grammar, parsing, SQL-specific validation, and translation into existing database operations |
| CLI | Interactive developer experience, SQL input, output formatting, and command conveniences |
| RPC/API | A stable, structured, typed request/response contract for applications; it must not require SQL text |
| Adapters | Translation between an external interface's model and the owning service's public Actions/contracts |

The SQL parser's AST and SQL-specific types are not universal database request types. They must not become dependencies of RDBService, storage services, or the RPC/API wire contract.

## Interface paths

The current SQL path is:

`SQL text → SQL parser / QES → RDB public Actions → database services → storage`

The intended future structured application path is:

`structured RPC/API request → API adapter / operation contract → owning public Actions → database services → storage`

The second path is a target architecture, not a claim that the complete RPC/API layer is already implemented.

Both paths may use the same underlying database capabilities and owning service Actions. They must not be forced through one another: RPC/API must not convert requests into SQL strings, invoke the SQL parser, or depend on QES.

## Rules for future design and implementation

1. **No SQL wire protocol requirement.** RPC/API requests use structured fields, typed values, explicit operations, and structured errors. SQL strings may be offered as an optional separate endpoint in the future, but never as the only application interface.
2. **No QES dependency from RPC/API.** The SQL execution implementation is not the general-purpose API service. An API adapter must call the appropriate owning public Actions or a separately designed operation layer.
3. **Keep database semantics with their owner.** Data integrity, type rules that apply to stored data, filtering capabilities, joins, and other reusable database behavior must not exist only as SQL parser tricks or QES-only code. Put each capability in the correct owning service/domain boundary, or explicitly design a reusable operation contract.
4. **SQL syntax stays at the edge.** Keywords, SQL AST nodes, aliases, SQL parser errors, and SQL formatting belong to the SQL interface. Do not leak them into storage or service contracts unless a specific owning-service design justifies it.
5. **Adapters translate; they do not own core behavior.** SQL and RPC/API adapters translate their input models into operations. They must not duplicate the database's actual data rules.
6. **Do not build capabilities solely to satisfy syntax.** Prioritize database capabilities for their value to UniDB. Add SQL syntax as one way for developers to access those capabilities.
7. **Do not duplicate implementations.** If SQL and RPC/API need the same operation, they should converge on the same owning Action/domain capability rather than maintaining two independent implementations.
8. **Keep service boundaries intact.** QES continues to use only RDBService's public Action-backed port. RPC/API must also use public contracts/Actions; neither interface may reach into CatalogService, TupleService, or storage internals.
9. **Storage is language-neutral.** Storage and tuple layers persist database-owned representations and opaque bytes. They do not parse SQL or understand SQL statements.
10. **Keep the interfaces independently testable.** SQL tests verify parsing and SQL-to-operation translation. RPC/API tests verify structured request handling and operation translation. Database capability tests verify the owning service behavior independently of either interface.

## How to evaluate a proposed feature

Before adding a SQL feature, ask:

- Is this a new database capability, or only another way to express an existing capability?
- If it is a database capability, which service/domain owns it independently of SQL?
- Can an application eventually request the same operation through structured RPC/API fields?
- Does the change accidentally make an SQL AST/type/parser part of a service or API contract?
- Does the feature require a real database capability, or are we changing core architecture just to imitate SQL syntax?

A SQL-only convenience can remain in the SQL interface. A database capability must not be trapped there.

## Current implementation note

Today, `QueryExecutionService` parses and executes the supported SQL/UQL subset through RDBService's public Actions. That is the current implementation of the developer SQL path. Its existence does not make SQL or QES a permanent dependency of the database core, and it does not define the future RPC/API contract.

This principle does not require an immediate rewrite of QES. Existing SQL support can continue to grow while reusable capabilities are kept in the correct service/domain ownership boundaries.
