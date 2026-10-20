#!/bin/bash

find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
  -not -path "./build/*" \
  -exec clang-format -i {} +

echo "Formatting complete."
