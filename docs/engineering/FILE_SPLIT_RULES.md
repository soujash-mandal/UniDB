# UniDB File Split Rules

**Status:** Proposed  
**Scope:** Entire UniDB repository  
**Audience:** All contributors, including human developers and AI agents

## 1. Purpose

Keep source files understandable, cohesive, and maintainable as UniDB grows.

These rules are intended to prevent files from accumulating unrelated responsibilities. They are not a target to make every file as small as possible.

**Primary principle: split by responsibility, not merely by line count.**

## 2. File size thresholds

The following thresholds are review limits, not a measure of code quality by themselves.

| File category | Preferred | Warning | Split required |
| --- | ---: | ---: | ---: |
| C++ implementation (`.cpp`) | ≤ 250 lines | 301–500 lines | > 500 lines |
| C++ header (`.h`, `.hpp`) | ≤ 150 lines | 201–350 lines | > 350 lines |
| Test implementation | ≤ 300 lines | 401–600 lines | > 600 lines |
| Test header | ≤ 150 lines | 201–350 lines | > 350 lines |

A file in the warning range must be reviewed when it is materially changed. A file above the split-required threshold must be split unless an exception is approved and documented.

### Counting lines

For consistent reviews:

- Count non-empty physical lines.
- Include comment-only lines; this keeps manual and automated counts deterministic.
- Exclude generated files, vendored dependencies, build output, and other files that UniDB does not maintain as source.
- Apply the test thresholds to test files, not the implementation thresholds.

The preferred size is a guideline, not a requirement to split a cohesive file.

## 3. Responsibility and cohesion

A file should have one clear primary responsibility.

Review a file for splitting when it:

- Implements multiple unrelated responsibilities or workflows.
- Contains unrelated classes that change for different reasons.
- Mixes orchestration with independent business logic that can be understood and tested separately.
- Combines unrelated test suites or scenarios.
- Has become difficult to navigate because distinct concepts are interleaved.

Related helper functions, private implementation details, and closely coupled operations may remain together when that makes the code easier to understand.

Do not split a cohesive implementation just because it crosses the preferred line count.

## 4. Class and declaration organization

- A substantial class should generally have a dedicated header and implementation file, following the conventions of its owning module.
- Small, tightly coupled helper types may share a file with the implementation they support.
- Avoid placing multiple unrelated public classes in one file.
- Keep implementation details out of public headers unless they are required by the interface or language constraints.
- Do not create a separate file for every small helper, function, or type without a clear navigation or ownership benefit.

## 5. Function-level review

Function count is a review signal, not an automatic splitting rule.

Review a file when it contains more than 15 functions, or when one or more functions are unusually long or difficult to follow.

Consider extracting a group of functions when they form a cohesive, independently understandable responsibility. Keep functions together when they operate on the same concept and separating them would make navigation harder.

## 6. Test file organization

Test files should be grouped by component, behavior, or scenario.

- Keep related test cases together.
- Split large suites when they cover distinct behaviors or become difficult to navigate.
- Prefer focused test files over one large catch-all test file.
- Preserve test coverage and test intent during any split.
- Do not create a separate file for every individual test case.

## 7. Safe splitting requirements

A file split is a refactoring and must preserve existing behavior.

A split must not:

- Change public behavior or interfaces unintentionally.
- Violate UniDB service ownership or dependency boundaries.
- Introduce cross-service imports or dependencies that were not previously allowed.
- Duplicate logic or create unnecessary abstraction layers.
- Expose private implementation details through public headers.
- Reduce or remove test coverage.

Follow the owning module's existing naming, namespace, include, and directory conventions.

## 8. Exceptions

A file may exceed a split-required threshold when splitting would make the design less cohesive or introduce unnecessary complexity.

Document the exception in the relevant PR with:

- File path and current line count.
- Reason the file should remain intact.
- Why splitting would make the code harder to maintain.
- Project-owner approval.
- A review date.

An exception is specific to the file and rationale documented; it is not a blanket exemption for the directory or module.

## 9. Adoption and enforcement

Adopt these rules incrementally:

1. Existing oversized files are not automatically required to change in unrelated PRs.
2. When an existing oversized file is materially modified, the contributor should assess whether the change is a good opportunity to split it.
3. New files must stay within the split-required thresholds unless an exception is approved.
4. A future automated repository check may report preferred-size warnings and enforce split-required limits.
5. Enforcement must support documented exceptions and must not require unrelated cleanup in every PR.

Until an automated checker is introduced, reviewers apply these rules during code review.

## 10. Review checklist

When reviewing a large or growing file, ask:

- Does this file have one clear primary responsibility?
- Are unrelated classes, workflows, or test scenarios mixed together?
- Would a split make each resulting file easier to understand?
- Would the split preserve module ownership and dependency boundaries?
- Would the split avoid unnecessary indirection and tiny files?
- Are tests preserved and still easy to discover?

If splitting does not improve cohesion, navigation, or maintainability, keep the file together and explain the decision when it exceeds a mandatory threshold.

---

**Rule of thumb:** A good split makes each file easier to name, understand, test, and change independently. If it does not, it is probably not a useful split.
