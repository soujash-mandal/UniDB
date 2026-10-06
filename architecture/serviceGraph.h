#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Architecture {

using ServiceNode = std::string;
using ServiceEdge = std::pair<ServiceNode, ServiceNode>;

inline const std::vector<ServiceNode> SERVICE_NODES = {
    "DiskManager",         "FreePageManager", "BufferPoolManager",
    "FreeSpaceMapService", "TupleService",    "CatalogService",
    "RdbService",
};

inline const std::vector<ServiceEdge> SERVICE_EDGES = {
    {"FreePageManager", "DiskManager"},
    {"BufferPoolManager", "FreePageManager"},
    {"BufferPoolManager", "DiskManager"},

    {"FreeSpaceMapService", "BufferPoolManager"},
    {"TupleService", "BufferPoolManager"},
    {"CatalogService", "BufferPoolManager"},

    {"RdbService", "CatalogService"},
    {"RdbService", "TupleService"},
    {"RdbService", "FreeSpaceMapService"},
};

} // namespace Architecture