#!/bin/bash

python3 - "$@" <<'PY'

import os
import re
import sys
import subprocess

if len(sys.argv) < 2:
    print("Usage: ./context.sh <file-or-directory> [file-or-directory...]")
    sys.exit(1)

root = os.getcwd()

visited = set()
files = []

include_pattern = re.compile(
    r'^\s*#\s*include\s*[<"]([^">]+)[">]',
    re.MULTILINE
)

SOURCE_EXTENSIONS = {
    ".h",
    ".hpp",
    ".cpp",
    ".cc",
    ".cxx",
}


def is_project_file(path):
    return (
        os.path.isfile(path)
        and os.path.splitext(path)[1] in SOURCE_EXTENSIONS
    )


def resolve_include(include, current_file):
    """
    Resolve:

        #include "foo.h"

    First relative to the current file,
    then relative to project root.
    """

    # Relative to current file
    candidate = os.path.normpath(
        os.path.join(os.path.dirname(current_file), include)
    )

    if os.path.isfile(candidate):
        return os.path.abspath(candidate)

    # Relative to project root
    candidate = os.path.normpath(
        os.path.join(root, include)
    )

    if os.path.isfile(candidate):
        return os.path.abspath(candidate)

    return None


def collect(file_path):
    """
    Collect one file and recursively collect its project includes.
    """

    file_path = os.path.abspath(file_path)

    if file_path in visited:
        return

    if not os.path.isfile(file_path):
        return

    # Don't include files outside the project
    if not file_path.startswith(root + os.sep):
        return

    visited.add(file_path)
    files.append(file_path)

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
    except UnicodeDecodeError:
        return

    for include in include_pattern.findall(content):
        dependency = resolve_include(include, file_path)

        if dependency:
            collect(dependency)


def collect_directory(directory):
    """
    Collect all C/C++ source/header files inside a directory.
    """

    directory = os.path.abspath(directory)

    for current_root, dirs, filenames in os.walk(directory):

        # Ignore build/.git directories
        dirs[:] = [
            d for d in dirs
            if d not in {
                ".git",
                "build",
                ".cache",
            }
        ]

        for filename in sorted(filenames):

            file_path = os.path.join(current_root, filename)

            if is_project_file(file_path):
                collect(file_path)


# Process every argument
for path in sys.argv[1:]:

    path = os.path.abspath(path)

    if os.path.isfile(path):
        collect(path)

    elif os.path.isdir(path):
        collect_directory(path)

    else:
        print(f"File or directory not found: {path}")
        sys.exit(1)


# Sort for deterministic output
files.sort()


# Build output
output = []

output.append("=" * 80)
output.append("PROJECT CONTEXT")
output.append("=" * 80)
output.append(f"Root: {root}")
output.append(f"Entries: {len(sys.argv) - 1}")
output.append(f"Files included: {len(files)}")
output.append("")


for i, file_path in enumerate(files, 1):

    relative_path = os.path.relpath(file_path, root)

    output.append("=" * 80)
    output.append(f"FILE {i}: {relative_path}")
    output.append("=" * 80)
    output.append("")

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            output.append(f.read())

    except Exception as e:
        output.append(f"[Could not read file: {e}]")

    output.append("")
    output.append("")


result = "\n".join(output)


# Copy to macOS clipboard
process = subprocess.Popen(
    ["pbcopy"],
    stdin=subprocess.PIPE
)

process.communicate(result.encode("utf-8"))


print(f"Copied {len(files)} files to clipboard.")
print("You can now paste the context into ChatGPT.")
print()

for file_path in files:
    print("  " + os.path.relpath(file_path, root))

PY