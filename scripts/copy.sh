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

if [ -f "$TARGET" ]; then
    cat "$TARGET" | pbcopy
    echo "Copied: $TARGET"
    exit 0
fi

if [ -d "$TARGET" ]; then
    {
        find "$TARGET" -type f | sort | while read -r FILE; do
            echo "================================================================================"
            echo "FILE: $FILE"
            echo "================================================================================"
            cat "$FILE"
            echo
        done
    } | pbcopy

    echo "Copied all files from: $TARGET"
    exit 0
fi

echo "Error: '$TARGET' is not a regular file or directory."
exit 1
