#!/usr/bin/env python3
"""Enforce that service type/ directories do not import project files.

Standard/system headers written with angle brackets are allowed. Quoted includes
are rejected, and angle-bracket includes are rejected when they resolve to a
file inside the repository.

Run from the repository root:
    python3 architecture/typeFolderImportTest.py
"""

from pathlib import Path
import re
import unittest
from tempfile import TemporaryDirectory

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
SOURCE_SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx"}
INCLUDE_PATTERN = re.compile(
    r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.MULTILINE
)
EXCLUDED_DIRECTORIES = {
    ".git", ".github", ".vscode", "architecture", "build", "container",
    "scripts",
}


def type_directories(root):
    """Return service-owned type/ directories at the repository's top level."""
    return sorted(
        child / "type"
        for child in root.iterdir()
        if child.is_dir()
        and child.name not in EXCLUDED_DIRECTORIES
        and not child.name.startswith(".")
        and (child / "type").is_dir()
    )


def resolves_inside_repository(root, service_root, source_path, include):
    """Check common project include roots without guessing compiler flags."""
    root = root.resolve()
    candidates = (
        source_path.parent / include,
        service_root / include,
        root / include,
    )
    for candidate in candidates:
        resolved = candidate.resolve()
        try:
            resolved.relative_to(root)
        except ValueError:
            continue
        if resolved.is_file():
            return True
    return False


def find_violations(root):
    """Find project-file includes in every service's type/ directory."""
    root = root.resolve()
    violations = []

    for type_directory in type_directories(root):
        service_root = type_directory.parent
        for source_path in sorted(type_directory.rglob("*")):
            if not source_path.is_file() or source_path.suffix not in SOURCE_SUFFIXES:
                continue

            try:
                source = source_path.read_text(encoding="utf-8")
            except (OSError, UnicodeError) as error:
                violations.append(
                    f"{source_path.relative_to(root)} could not be read: {error}"
                )
                continue

            for delimiter, include in INCLUDE_PATTERN.findall(source):
                is_project_include = delimiter == '"'
                if delimiter == "<":
                    is_project_include = resolves_inside_repository(
                        root, service_root, source_path, include
                    )

                if is_project_include:
                    violations.append(
                        f'{source_path.relative_to(root)} imports "{include}"; '
                        "type/ files must not include project files"
                    )

    return violations


class TypeFolderImportTest(unittest.TestCase):
    def test_repository_type_folders_do_not_import_project_files(self):
        violations = find_violations(REPOSITORY_ROOT)
        self.assertEqual(
            violations,
            [],
            "Files inside service type/ directories must not import project files:\n"
            + "\n".join(violations),
        )

    def test_detector_rejects_quoted_project_include(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "ExampleService" / "type" / "type.h"
            source.parent.mkdir(parents=True)
            source.write_text('#include "../domain/Thing.h"\n', encoding="utf-8")

            self.assertEqual(
                find_violations(root),
                [
                    'ExampleService/type/type.h imports "../domain/Thing.h"; '
                    "type/ files must not include project files"
                ],
            )

    def test_detector_rejects_angle_bracket_project_include(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "ExampleService" / "type" / "type.h"
            target = root / "ExampleService" / "domain" / "Thing.h"
            source.parent.mkdir(parents=True)
            target.parent.mkdir(parents=True)
            source.write_text("#include <domain/Thing.h>\n", encoding="utf-8")
            target.write_text("#pragma once\n", encoding="utf-8")

            self.assertEqual(len(find_violations(root)), 1)

    def test_detector_allows_standard_library_includes(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "ExampleService" / "type" / "type.h"
            source.parent.mkdir(parents=True)
            source.write_text(
                "#include <cstdint>\n#include <string>\n"
                "#pragma once\n",
                encoding="utf-8",
            )

            self.assertEqual(find_violations(root), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
