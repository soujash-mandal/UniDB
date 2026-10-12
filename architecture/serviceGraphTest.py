#!/usr/bin/env python3
"""Validate that the declared service graph matches the repository's service inventory.

Run from the repository root:
    python3 architecture/serviceGraphTest.py
"""

from pathlib import Path
import re
import unittest
from tempfile import TemporaryDirectory

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
GRAPH_PATH = REPOSITORY_ROOT / "architecture" / "serviceGraph.h"
SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}
SERVICE_MARKERS = ("actions", "domain", "port", "adapter", "adapters")
EXCLUDED_DIRECTORIES = {
    ".git", ".github", ".vscode", "architecture", "build", "container",
    "core", "scripts",
}

NODE_BLOCK = re.compile(
    r"SERVICE_NODES\s*=\s*\{(?P<body>.*?)\};", re.DOTALL
)
EDGE_BLOCK = re.compile(
    r"SERVICE_EDGES\s*=\s*\{(?P<body>.*?)\};", re.DOTALL
)
STRING = re.compile(r'"([^"]+)"')
EDGE = re.compile(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}')


def discover_services(root):
    """Discover top-level service directories using their conventional layers."""
    return {
        path.name
        for path in root.iterdir()
        if path.is_dir()
        and path.name not in EXCLUDED_DIRECTORIES
        and not path.name.startswith(".")
        and any((path / marker).is_dir() for marker in SERVICE_MARKERS)
    }


def parse_graph(source):
    """Return (nodes, edges), raising ValueError for an unparseable graph."""
    node_match = NODE_BLOCK.search(source)
    edge_match = EDGE_BLOCK.search(source)
    if not node_match or not edge_match:
        raise ValueError("Could not parse SERVICE_NODES and SERVICE_EDGES in serviceGraph.h")

    nodes = STRING.findall(node_match.group("body"))
    edges = EDGE.findall(edge_match.group("body"))
    # Remove edge expressions before looking for stray quoted node strings.
    return nodes, edges


def include_dependencies(root, services):
    """Find service-to-service dependencies expressed as project includes.

    This intentionally checks explicit cross-service include paths, not arbitrary
    mentions of service names. Runtime wiring and non-include dependencies still
    require graph review and are not inferred by this check.
    """
    dependencies = set()
    for source_name in services:
        source_dir = root / source_name
        for path in source_dir.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            try:
                source = path.read_text(encoding="utf-8")
            except (OSError, UnicodeError):
                continue
            for include in re.findall(
                r'^\s*#\s*include\s*["<]([^">]+)[">]', source, re.MULTILINE
            ):
                candidate = (path.parent / include).resolve()
                try:
                    relative = candidate.relative_to(root.resolve())
                except ValueError:
                    continue
                if relative.parts and relative.parts[0] in services:
                    target_name = relative.parts[0]
                    if target_name != source_name:
                        dependencies.add((source_name, target_name))
    return dependencies


def validate_graph(source, services, root):
    nodes, edges = parse_graph(source)
    errors = []

    if len(nodes) != len(set(nodes)):
        errors.append("SERVICE_NODES contains duplicate nodes")
    if len(edges) != len(set(edges)):
        errors.append("SERVICE_EDGES contains duplicate edges")

    node_set = set(nodes)
    for source_name, target_name in edges:
        if source_name not in node_set:
            errors.append(f"Edge source is not a declared node: {source_name}")
        if target_name not in node_set:
            errors.append(f"Edge target is not a declared node: {target_name}")
        if source_name == target_name:
            errors.append(f"Self-dependency is not allowed: {source_name}")

    for service in sorted(services - node_set):
        errors.append(f"Service directory is missing from SERVICE_NODES: {service}")

    for node in sorted(node_set - services):
        errors.append(f"Graph node has no discovered service directory: {node}")

    declared_edges = set(edges)
    for source_name, target_name in sorted(include_dependencies(root, services) - declared_edges):
        errors.append(
            f"Cross-service include missing from SERVICE_EDGES: "
            f"{source_name} -> {target_name}"
        )

    return errors


class ServiceGraphTest(unittest.TestCase):
    def test_repository_service_graph_is_consistent(self):
        errors = validate_graph(
            GRAPH_PATH.read_text(encoding="utf-8"),
            discover_services(REPOSITORY_ROOT),
            REPOSITORY_ROOT,
        )
        self.assertEqual(errors, [], "\n".join(errors))

    def test_duplicate_nodes_are_rejected(self):
        source = '''
        SERVICE_NODES = { "ExampleService", "ExampleService" };
        SERVICE_EDGES = { };
        '''
        errors = validate_graph(source, {"ExampleService"}, Path("/tmp"))
        self.assertIn("SERVICE_NODES contains duplicate nodes", errors)

    def test_duplicate_edges_are_rejected(self):
        source = '''
        SERVICE_NODES = { "A", "B" };
        SERVICE_EDGES = { {"A", "B"}, {"A", "B"} };
        '''
        errors = validate_graph(source, {"A", "B"}, Path("/tmp"))
        self.assertIn("SERVICE_EDGES contains duplicate edges", errors)

    def test_missing_service_node_is_rejected(self):
        source = '''
        SERVICE_NODES = { "ExistingService" };
        SERVICE_EDGES = { };
        '''
        errors = validate_graph(
            source, {"ExistingService", "NewService"}, Path("/tmp")
        )
        self.assertIn(
            "Service directory is missing from SERVICE_NODES: NewService", errors
        )

    def test_missing_cross_service_include_edge_is_rejected(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "ServiceA" / "actions" / "Run.h"
            target = root / "ServiceB" / "domain" / "Thing.h"
            source.parent.mkdir(parents=True)
            target.parent.mkdir(parents=True)
            source.write_text(
                '#include "../../ServiceB/domain/Thing.h"\n', encoding="utf-8"
            )
            target.write_text("#pragma once\n", encoding="utf-8")
            graph = '''
            SERVICE_NODES = { "ServiceA", "ServiceB" };
            SERVICE_EDGES = { };
            '''
            errors = validate_graph(graph, {"ServiceA", "ServiceB"}, root)
            self.assertIn(
                "Cross-service include missing from SERVICE_EDGES: ServiceA -> ServiceB",
                errors,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
