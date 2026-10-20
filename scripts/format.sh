#!/bin/bash
set -euo pipefail

if ! command -v clang-format >/dev/null 2>&1; then
  echo "ERROR: clang-format is required. Install it before formatting." >&2
  exit 1
fi

find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
  -not -path "./build/*" \
  -exec clang-format -i {} +

echo "Formatting complete."
