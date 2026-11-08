# UniDB Agent Instructions

These instructions apply to all work in this repository.

## Required workflow for commits, pushes, and pull requests

1. **Format the code.** Run `./scripts/format.sh` after making or modifying C++ source/header files and before committing or pushing. This script applies `clang-format` in place.
2. **Review formatting changes.** Inspect `git diff` after formatting. Keep intended formatting changes, and investigate any unrelated changes rather than blindly including them.
3. **Build and test.** Use the repository's documented build and test workflow (including `./scripts/test.sh` when applicable). Run the relevant tests for the change.
4. **Validate lint and CI requirements.** Run available local checks before committing or pushing. After the push to `main` or pull-request update that triggers GitHub Actions, inspect the results for that revision. Formatting does not fix every kind of lint failure: compiler diagnostics, static-analysis findings, build failures, and test failures may need separate code changes.
5. **Inspect the final changes.** Review `git diff` and `git status` to ensure only intended files are changed and no generated or unrelated files are included.
6. **Fix failures before pushing.** Do not ignore, suppress, or bypass a failing check just to get a green result. Diagnose the cause, fix it, and rerun the relevant checks.
7. **Report accurately.** Do not describe a PR as ready until required CI checks have passed and applicable review-bot feedback (including CodeRabbit) has been addressed. Clearly report any remaining blocker.

## Scope and repository conventions

- Keep changes focused on the approved Linear task.
- Follow the existing service boundaries and architecture documented in the repository.
- Do not change CI workflows, formatting configuration, or unrelated files unless the task explicitly requires it.
- Do not merge a PR; leave final review and merge approval to the repository owner.

## Type folder isolation

- Every service may have a `type/` directory as a sibling of `domain/`.
- Files inside a service's `type/` directory must not include project-owned files.
- Standard and system headers are allowed using angle-bracket includes (for example, `#include <string>`).
- Quoted includes are forbidden inside `type/`; angle-bracket includes that resolve to repository files are also forbidden.
- Enforce this rule with `python3 architecture/typeFolderImportTest.py`. The check runs in the GitHub Actions test workflow.
- Keep type declarations self-contained. Do not move another module's declarations into a service's `type/` directory merely to avoid an include; agree on ownership and redesign the shared contract instead.
