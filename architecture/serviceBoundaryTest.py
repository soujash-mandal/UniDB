#!/usr/bin/env python3
"""Enforce service import boundaries for Actions, Ports, and Domain.

Run from the repository root:
    python3 architecture/serviceBoundaryTest.py
"""

from pathlib import Path
import re
import unittest

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
PROTECTED_LAYERS = ("actions", "port", "domain")
SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
EXCLUDED_DIRECTORIES = {
    ".git", ".github", ".vscode", "architecture", "build", "container",
    "core", "scripts",
}


def service_directories(root):
    """Discover top-level service directories by their protected layer folders."""
    services = []
    for directory in root.iterdir():
        if (
            directory.is_dir()
            and directory.name not in EXCLUDED_DIRECTORIES
            and not directory.name.startswith(".")
            and any((directory / layer).is_dir() for layer in PROTECTED_LAYERS)
        ):
            services.append(directory)
    return sorted(services)


def find_violations(root):
    """Return cross-service includes found in protected source files."""
    root = root.resolve()
    services = service_directories(root)
    service_names = {service.name for service in services}
    violations = []

    for service in services:
        for layer in PROTECTED_LAYERS:
            layer_directory = service / layer
            if not layer_directory.is_dir():
                continue

            for source_path in sorted(layer_directory.rglob("*")):
                if not source_path.is_file() or source_path.suffix not in SOURCE_SUFFIXES:
                    continue

                source = source_path.read_text(encoding="utf-8")
                for include in INCLUDE_PATTERN.findall(source):
                    # Resolve quoted/project includes relative to this source file.
                    # System includes and unresolved includes are outside this rule.
                    candidate = (source_path.parent / include).resolve()
                    try:
                        relative = candidate.relative_to(root)
                    except ValueError:
                        continue

                    if relative.parts and relative.parts[0] in service_names:
                        imported_service = relative.parts[0]
                        if imported_service != service.name:
                            violations.append(
                                f'{source_path.relative_to(root)} imports "{include}" '
                                f'from service {imported_service}'
                            )

    return violations


class ServiceBoundaryTest(unittest.TestCase):
    def test_repository_has_no_cross_service_imports_in_protected_layers(self):
        violations = find_violations(REPOSITORY_ROOT)
        self.assertEqual(
            violations,
            [],
            "Actions, Ports, and Domain must not import another service's code:\n"
            + "\n".join(violations),
        )

    def test_detector_flags_cross_service_import(self):
        from tempfile import TemporaryDirectory

        with TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source = root / "ExampleService" / "actions" / "DoThing.h"
            target = root / "OtherService" / "domain" / "OtherType.h"
            source.parent.mkdir(parents=True)
            target.parent.mkdir(parents=True)
            source.write_text('#include "../../OtherService/domain/OtherType.h"\n', encoding="utf-8")
            target.write_text("#pragma once\n", encoding="utf-8")

            self.assertEqual(
                find_violations(root),
                [
                    'ExampleService/actions/DoThing.h imports '
                    '"../../OtherService/domain/OtherType.h" from service OtherService'
                ],
            )

    def test_detector_allows_same_service_import(self):
        from tempfile import TemporaryDirectory

        with TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source = root / "ExampleService" / "actions" / "DoThing.h"
            target = root / "ExampleService" / "domain" / "OwnType.h"
            source.parent.mkdir(parents=True)
            target.parent.mkdir(parents=True)
            source.write_text('#include "../domain/OwnType.h"\n', encoding="utf-8")
            target.write_text("#pragma once\n", encoding="utf-8")

            self.assertEqual(find_violations(root), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
