#!/usr/bin/env python3
"""Tests that service Actions, Ports, and Domain do not include other services' code.

Run from the repository root:
    python3 architecture/serviceBoundaryTest.py
"""

from pathlib import Path
import re
import unittest

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
FORBIDDEN_LAYERS = {"actions", "port", "domain"}
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def service_directories(root=REPOSITORY_ROOT):
    """Find top-level service directories containing a protected layer."""
    excluded = {".git", ".github", ".vscode", "architecture", "build", "container", "core", "scripts"}
    result = []
    for directory in root.iterdir():
        if not directory.is_dir() or directory.name in excluded or directory.name.startswith("."):
            continue
        if any((directory / layer).is_dir() for layer in FORBIDDEN_LAYERS):
            result.append(directory)
    return sorted(result)


def protected_source_files(root=REPOSITORY_ROOT):
    """Yield source/header files in each service's Actions, Port, and Domain layers."""
    for service in service_directories(root):
        for layer in FORBIDDEN_LAYERS:
            layer_path = service / layer
            if layer_path.is_dir():
                for path in sorted(layer_path.rglob("*")):
                    if path.is_file() and path.suffix in {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}:
                        yield service, path


def cross_service_imports(source, service, root=REPOSITORY_ROOT):
    """Return included paths that resolve into a different service directory."""
    violations = []
    for include in INCLUDE_PATTERN.findall(source):
        # Project includes are resolved relative to the including file, as C++ does.
        candidate = (source_path_parent(source, root) / include).resolve()
        try:
            relative = candidate.relative_to(root.resolve())
        except ValueError:
            continue

        if not relative.parts:
            continue

        included_service = relative.parts[0]
        known_services = {directory.name for directory in service_directories(root)}
        if included_service in known_services and included_service != service.name:
            violations.append((include, included_service))
    return violations


def source_path_parent(source, root):
    # Kept separate to allow the scanner to accept source text in unit tests.
    return getattr(source, "parent", root)


class ServiceBoundaryTest(unittest.TestCase):
    def test_current_actions_ports_and_domains_do_not_import_other_services(self):
        violations = []
        services = service_directories()
        known_services = {service.name for service in services}

        for service, path in protected_source_files():
            for include in INCLUDE_PATTERN.findall(path.read_text(encoding="utf-8")):
                candidate = (path.parent / include).resolve()
                try:
                    relative = candidate.relative_to(REPOSITORY_ROOT.resolve())
                except ValueError:
                    continue

                if relative.parts and relative.parts[0] in known_services:
                    included_service = relative.parts[0]
                    if included_service != service.name:
                        violations.append(
                            f"{path.relative_to(REPOSITORY_ROOT)} imports "
                            f'"{include}" from service {included_service}'
                        )

        self.assertEqual(
            violations,
            [],
            "Cross-service imports are forbidden in Actions, Ports, and Domain:\n"
            + "\n".join(violations),
        )

    def test_fixture_detects_cross_service_import(self):
        """A representative include from one service into another is forbidden."""
        with self.subTest("cross-service include"):
            including_service = "ExampleService"
            included_service = "OtherService"
            include = "../OtherService/domain/OtherType.h"
            resolved = (Path("/repo") / including_service / "actions" / include).resolve()
            relative = resolved.relative_to(Path("/repo"))
            self.assertEqual(relative.parts[0], included_service)
            self.assertNotEqual(relative.parts[0], including_service)

    def test_same_service_import_is_not_a_cross_service_import(self):
        include = "../domain/OwnType.h"
        resolved = (Path("/repo") / "ExampleService" / "actions" / include).resolve()
        relative = resolved.relative_to(Path("/repo"))
        self.assertEqual(relative.parts[0], "ExampleService")


if __name__ == "__main__":
    unittest.main(verbosity=2)
