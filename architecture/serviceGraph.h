#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Architecture {

using ServiceNode = std::string;
using ServiceEdge = std::pair<ServiceNode, ServiceNode>;

inline const std::vector<ServiceNode> SERVICE_NODES = {
    "DiskManager",         "FreePageManager",       "BufferPoolManager",
    "FreeSpaceMapService", "TupleService",          "CatalogService",
    "RdbMetadataService",  "QueryExecutionService", "RdbService",
};

inline const std::vector<ServiceEdge> SERVICE_EDGES = {
    {"FreePageManager", "DiskManager"},
    {"FreePageManager", "BufferPoolManager"},
    {"BufferPoolManager", "FreePageManager"},
    {"BufferPoolManager", "DiskManager"},

    {"FreeSpaceMapService", "BufferPoolManager"},
    {"FreeSpaceMapService", "TupleService"},
    {"TupleService", "BufferPoolManager"},
    {"TupleService", "FreePageManager"},
    {"CatalogService", "BufferPoolManager"},
    {"RdbMetadataService", "BufferPoolManager"},

    {"RdbService", "CatalogService"},
    {"RdbService", "TupleService"},
    {"RdbService", "RdbMetadataService"},
    {"QueryExecutionService", "RdbService"},
};

} // namespace Architecture