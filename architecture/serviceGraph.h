#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Architecture {

using ServiceNode = std::string;
using ServiceEdge = std::pair<ServiceNode, ServiceNode>;

inline const std::vector<ServiceNode> SERVICE_NODES = {
    "DiskManager",        "FreePageManager", "BufferPoolManager",
    "SpaceMapService",    "FreeSpaceMapService", "TupleService",
    "CatalogService",
    "RdbMetadataService", "RdbService",      "QueryExecutionService",
};

// An edge A -> B means service A depends on/uses service B.
// RdbService coordinates the RDB services. The legacy SpaceMapService
// includes TupleService, while FreeSpaceMapService is represented separately.
inline const std::vector<ServiceEdge> SERVICE_EDGES = {
    {"FreePageManager", "DiskManager"},
    {"BufferPoolManager", "FreePageManager"},
    {"BufferPoolManager", "DiskManager"},

    {"SpaceMapService", "BufferPoolManager"},
    {"SpaceMapService", "TupleService"},
    {"FreeSpaceMapService", "BufferPoolManager"},
    {"TupleService", "BufferPoolManager"},
    {"CatalogService", "BufferPoolManager"},
    {"RdbMetadataService", "BufferPoolManager"},

    {"RdbService", "CatalogService"},
    {"RdbService", "TupleService"},
    {"RdbService", "RdbMetadataService"},
    {"RdbService", "SpaceMapService"},
    {"QueryExecutionService", "RdbService"},
};

} // namespace Architecture
