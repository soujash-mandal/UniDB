# UniDB V1 Architecture

## Architecture overview

```mermaid
flowchart TD
    SQL["SQL Interface"]
    Query["Parser + Query Execution"]

    subgraph RDB_LAYER["Relational Database Layer"]
        RDB["RdbService"]
        Metadata["RdbMetadataService"]
        Catalog["CatalogService"]
        Tuple["TupleService"]
        SpaceMap["SpaceMapService (deferred from V1)"]
        RDB --> Catalog
        RDB --> Tuple
        RDB --> SpaceMap
        RDB --> Metadata
    end

    BPM["BufferPoolManager"]
    FPM["FreePageManager"]
    Disk["DiskManager"]
    Container["Container / Dependency Injection"]

    SQL --> Query
    Query --> RDB

    Catalog --> BPM
    Tuple --> BPM
    SpaceMap --> BPM
    Metadata --> BPM
    BPM --> FPM
    BPM --> Disk
    FPM --> Disk

    Container -. "wires Actions and concrete Adapters" .-> RDB
    Container -. "wires Actions and concrete Adapters" .-> Catalog
    Container -. "wires Actions and concrete Adapters" .-> Tuple
    Container -. "wires Actions and concrete Adapters" .-> SpaceMap
    Container -. "wires Actions and concrete Adapters" .-> Metadata
    Container -. "wires storage dependencies" .-> BPM
```

## Declared service dependency graph

This diagram mirrors the nodes and directed edges declared in
[`architecture/serviceGraph.h`](architecture/serviceGraph.h). An arrow
`A --> B` means service `A` depends on or uses service `B`.

```mermaid
flowchart TD
    RDB["RdbService"] --> Catalog["CatalogService"]
    RDB --> Tuple["TupleService"]
    RDB --> SpaceMap["SpaceMapService"]
    RDB --> Metadata["RdbMetadataService"]

    Catalog --> BPM["BufferPoolManager"]
    Tuple --> BPM
    SpaceMap --> BPM
    SpaceMap --> Tuple
    Metadata --> BPM

    BPM --> FPM["FreePageManager"]
    BPM --> Disk["DiskManager"]
    FPM --> Disk
```

**V1 scope note:** `SpaceMapService` is currently represented in the repository's
service graph, but remains deferred from V1 acceptance criteria unless a concrete V1
operation requires it. The `SpaceMapService --> TupleService` edge reflects the
current implementation's tuple-page creation port; it should be revisited when SpaceMapService's
page-selection and page-creation responsibilities are refactored.
