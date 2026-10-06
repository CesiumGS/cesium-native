---
name: prepare-release
description: 'Prepare a Cesium Native release for a given version number. Use when asked to prepare or cut a release, bump the release version, or run release preparation for cesium-native. Verifies CI on the canonical CesiumGS/cesium-native main branch, reviews and expands CHANGES.md, and sets the version in package.json and CMakeLists.txt.'
argument-hint: 'The release version number, for example 0.65.0'
---

# Prepare a Cesium Native release

Performs the first four steps of the release process documented in `doc/topics/release-process.md`.

The release version number is supplied as the argument to this skill. If no version number was given, ask the user for it before doing anything else. Everywhere below, "the release version" means that number, written without a leading `v`.

## Procedure

1. Check the latest GitHub Actions status for the canonical `CesiumGS/cesium-native` repository and its `main` branch using `gh`, but do not trust `branch=main` alone because a fork can also have a `main` branch. Use a repo-qualified query that filters to the canonical repo owner, for example: `gh api repos/CesiumGS/cesium-native/actions/runs --paginate --jq '.workflow_runs[] | select(.head_repository.owner.login == "CesiumGS" and .head_branch == "main") | {name, status, conclusion, created_at, head_repository: .head_repository.full_name}' | head -n 5`. Inspect the most recent relevant `push`/CI workflow run and confirm it is green before continuing. If the latest relevant canonical main-branch run is failing, fix the root cause before continuing. Do not proceed with the release changes while the canonical main branch is red.
2. Always review and update `CHANGES.md` before changing any version numbers. This step is mandatory and blocking. Diff against the previous release, inspect all changes since then, and ensure the newest section is accurate, complete, and has the correct version header and date for the release version. Review the full set of changes since the last release and expand the changelog if necessary so every notable change is represented; do not leave a placeholder, summary-only note, or partial changelog. Use the next first business day of the month for the date in the changelog header (for example, if today is 10/29, use 11/1 if it is a Monday; otherwise use the first Monday of the month). If the changelog is missing, stale, or clearly incomplete, fix it before proceeding. Never update the package or CMake version until the changelog has been reviewed, corrected, and verified complete.
3. Update the `version` field in `package.json` to the release version.
4. Update the `VERSION` argument passed to `project()` in `CMakeLists.txt` to the release version.
5. Commit only if the user explicitly asks for it.

Before finishing, give a brief summary of the files changed and call out any remaining issues that need manual review or confirmation.
