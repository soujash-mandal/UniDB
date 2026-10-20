# UniDB Agent Instructions

These instructions apply to all work in this repository.

## Required workflow for commits, pushes, and pull requests

1. **Install the repository Git hooks once per clone.** Run `./scripts/install-hooks.sh`. This configures Git to use the versioned `.githooks/` directory.
2. **Format the code.** Run `./scripts/format.sh` after making or modifying C++ source/header files and before committing or pushing. The installed pre-commit hook also runs this command automatically before every commit.
3. **Review formatting changes.** Inspect `git diff` after formatting. Keep intended formatting changes, and investigate any unrelated changes rather than blindly including them. If the pre-commit hook reports that formatting changed files, review and stage the intended formatting changes, then retry the commit.
4. **Build and test.** Use the repository's documented build and test workflow (including `./scripts/test.sh` when applicable). Run the relevant tests for the change.
5. **Validate lint and CI requirements.** Run available local checks before committing or pushing. After each pull-request update that triggers GitHub Actions, inspect the results for that revision. Formatting does not fix every kind of lint failure: compiler diagnostics, static-analysis findings, build failures, and test failures may need separate code changes.
6. **Inspect the final changes.** Review `git diff` and `git status` to ensure only intended files are changed and no generated or unrelated files are included.
7. **Fix failures before pushing.** Do not ignore, suppress, or bypass a failing check just to get a green result. Diagnose the cause, fix it, and rerun the relevant checks.
8. **Report accurately.** Do not describe a PR as ready until required CI checks have passed and applicable review-bot feedback (including CodeRabbit) has been addressed. Clearly report any remaining blocker.

## Scope and repository conventions

- Keep changes focused on the approved Linear task.
- Follow the existing service boundaries and architecture documented in the repository.
- Do not change CI workflows, formatting configuration, or unrelated files unless the task explicitly requires it.
- Do not merge a PR; leave final review and merge approval to the repository owner.
