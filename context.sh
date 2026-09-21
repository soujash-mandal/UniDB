#!/bin/bash

python3 - "$@" <<'PY'
import os
import re
import sys
import subprocess

if len(sys.argv) != 2:
    print("Usage: ./context.sh <file>")
    sys.exit(1)

root = os.getcwd()
start_file = os.path.abspath(sys.argv[1])

if not os.path.isfile(start_file):
    print(f"File not found: {sys.argv[1]}")
    sys.exit(1)

visited = set()
files = []

include_pattern = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]', re.MULTILINE)


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


collect(start_file)

output = []

output.append("=" * 80)
output.append("PROJECT CONTEXT")
output.append("=" * 80)
output.append(f"Root: {root}")
output.append(f"Entry file: {os.path.relpath(start_file, root)}")
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