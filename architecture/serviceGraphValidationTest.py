#!/usr/bin/env python3
"""Validate the declared service graph against service-level source includes.

Run from the repository root:
    python3 architecture/serviceGraphValidationTest.py
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parent.parent
GRAPH_HEADER = ROOT / "architecture" / "serviceGraph.h"
SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
NODE_BLOCK = re.compile(r"SERVICE_NODES\s*=\s*\{(.*?)\};", re.DOTALL)
EDGE_BLOCK = re.compile(r"SERVICE_EDGES\s*=\s*\{(.*?)\};", re.DOTALL)
STRING_PATTERN = re.compile(r'"([^"]+)"')
EDGE_PATTERN = re.compile(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}')
EXCLUDED_DIRECTORIES = {
    ".git", ".github", ".vscode", "architecture", "build", "container",
    "core", "scripts",
}
SERVICE_LAYERS = ("actions", "domain", "port", "adapter", "adapters")


def discover_services(root=ROOT):
    """Find top-level directories that have a conventional service layer."""
    return {
        path.name
        for path in root.iterdir()
        if path.is_dir()
        and path.name not in EXCLUDED_DIRECTORIES
        and not path.name.startswith(".")
        and any((path / layer).is_dir() for layer in SERVICE_LAYERS)
    }


def parse_graph(header=GRAPH_HEADER):
    """Read node and edge declarations from serviceGraph.h."""
    source = header.read_text(encoding="utf-8")
    node_match = NODE_BLOCK.search(source)
    edge_match = EDGE_BLOCK.search(source)
    if not node_match or not edge_match:
        raise ValueError("Could not parse SERVICE_NODES and SERVICE_EDGES in serviceGraph.h")
    nodes = STRING_PATTERN.findall(node_match.group(1))
    edges = EDGE_PATTERN.findall(edge_match.group(1))
    return nodes, edges


def source_include_dependencies(root=ROOT):
    """Find cross-service dependencies expressed as resolvable project includes."""
    root = root.resolve()
    services = discover_services(root)
    dependencies = set()
    for source_service in services:
        service_root = root / source_service
        for path in service_root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            try:
                source = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            for include in INCLUDE_PATTERN.findall(source):
                target = (path.parent / include).resolve()
                try:
                    relative = target.relative_to(root)
                except ValueError:
                    continue
                if relative.parts and relative.parts[0] in services:
                    target_service = relative.parts[0]
                    if target_service != source_service:
                        dependencies.add((source_service, target_service))
    return dependencies


class ServiceGraphValidationTest(unittest.TestCase):
    def test_graph_nodes_are_unique(self):
        nodes, _ = parse_graph()
        self.assertEqual(len(nodes), len(set(nodes)), "Duplicate service node in serviceGraph.h")

    def test_graph_edges_are_unique(self):
        _, edges = parse_graph()
        self.assertEqual(len(edges), len(set(edges)), "Duplicate service edge in serviceGraph.h")

    def test_every_edge_endpoint_is_declared(self):
        nodes, edges = parse_graph()
        node_set = set(nodes)
        invalid = [(source, target) for source, target in edges
                   if source not in node_set or target not in node_set]
        self.assertEqual(invalid, [], f"Edges reference undeclared nodes: {invalid}")

    def test_discovered_services_are_declared_in_graph(self):
        nodes, _ = parse_graph()
        missing = sorted(discover_services() - set(nodes))
        self.assertEqual(
            missing, [],
            "Service directories are missing from architecture/serviceGraph.h: "
            + ", ".join(missing),
        )

    def test_cross_service_source_includes_are_graph_edges(self):
        nodes, edges = parse_graph()
        node_set = set(nodes)
        dependencies = source_include_dependencies()
        unlisted = sorted(dependency for dependency in dependencies
                          if dependency[0] in node_set and dependency[1] in node_set
                          and dependency not in set(edges))
        self.assertEqual(
            unlisted, [],
            "Cross-service includes missing from SERVICE_EDGES: "
            + ", ".join(f"{source} -> {target}" for source, target in unlisted),
        )

    def test_fixture_detects_missing_edge_and_invalid_endpoint(self):
        nodes = ["Alpha", "Beta"]
        edges = [("Alpha", "Missing")]
        node_set = set(nodes)
        invalid = [edge for edge in edges if edge[0] not in node_set or edge[1] not in node_set]
        self.assertEqual(invalid, [("Alpha", "Missing")])
        declared_edges = {("Alpha", "Beta")}
        actual_dependency = ("Alpha", "Beta")
        self.assertNotIn(actual_dependency, set(edges))
        self.assertIn(actual_dependency, declared_edges)

    def test_intentional_ci_failure_probe(self):
        """Temporary probe: this test must fail to verify branch-protection checks."""
        self.fail("Intentional CI failure probe; remove this test after verification.")


if __name__ == "__main__":
    unittest.main(verbosity=2)
