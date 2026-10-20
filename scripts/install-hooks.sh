#!/bin/bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

chmod +x .githooks/pre-commit
git update-index --chmod=+x .githooks/pre-commit
git config --local core.hooksPath .githooks

echo "Installed UniDB Git hooks: core.hooksPath=.githooks"
echo "The pre-commit hook will run ./scripts/format.sh before every commit."
