#!/bin/bash

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <file-or-folder>"
    exit 1
fi

TARGET="$1"

if [ ! -e "$TARGET" ]; then
    echo "Error: '$TARGET' does not exist."
    exit 1
fi

FILES=()

if [ -f "$TARGET" ]; then
    FILES+=("$TARGET")
else
    while IFS= read -r FILE; do
        FILES+=("$FILE")
    done < <(find "$TARGET" -type f | sort)
fi

if [ ${#FILES[@]} -eq 0 ]; then
    echo "No files found."
    exit 0
fi

{
    echo "================================================================================"
    echo "FILES COPIED: ${#FILES[@]}"
    echo "SOURCE: $TARGET"
    echo "================================================================================"
    echo

    for FILE in "${FILES[@]}"; do
        echo "================================================================================"
        echo "FILE: $FILE"
        echo "================================================================================"
        cat "$FILE"
        echo
        echo
    done
} | pbcopy

echo "================================================================================"
echo "Copied ${#FILES[@]} file(s) to clipboard"
echo "================================================================================"
echo

for FILE in "${FILES[@]}"; do
    echo "$FILE"
done