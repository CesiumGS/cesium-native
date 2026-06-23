# Releasing a new version of Cesium Native {#native-release-process}

This is the process we follow when releasing a new version of Cesium Native.

## Prepare for Release

1. Verify that CI is passing on all platforms. Fix it if not.
2. Verify that `CHANGES.md` is complete and accurate.
   - Give the header of the section containing the latest changes an appropriate version number and date.
   - Diff main against the previous released version. This helps catch changes that are missing from the changelog, as well as changelog entries that were accidentally added to the wrong section.
3. Set the `version` property in `package.json`.
4. Set the `VERSION` property passed to the `project()` function in `CMakeLists.txt`.
5. Commit these changes. You can push them directly to `main`.

## Release

Releasing is simply a matter of tagging the release and pushing the tag to GitHub. Replace the version number with the one used in the preparation steps above.

1. `git tag -a v0.56.0 -m "0.56.0 release"`
2. `git push origin v0.56.0`

## Publish Updated Reference Docs

A CI job automatically publishes the documentation to the web site at https://cesium.com/learn/cesium-native/ref-doc/ when it is merged into the [cesium.com](https://github.com/CesiumGS/cesium-native/tree/cesium.com) branch. So do the following:

1. `git checkout cesium.com`
2. `git pull --ff-only`
3. `git merge v0.56.0 --ff-only`
4. `git push`
5. `git checkout main`

The `--ff-only` flags ensure that no new commits are created in the process. We only want to bring over commits from the release tag. If the pull or merge fails, something unusual has probably happened with the git history and it warrants a little investigation before proceeding.

# Release Schedule

## Release Coordinator

| Date      | User             |
| --------- | ---------------- |
| 6/1/2026  | `@timoore`       |
| 7/1/2026  | `@azrogers`      |
| 8/3/2026  | `@david-lively`  |
| 9/1/2026  | `@j9liu`         |
| 10/1/2026 | `@timoore`       |
| 11/2/2026 | `@azrogers`      |
| 12/1/2026 | `@david-lively`  |

## Cesium Native Runtimes Release Timeline

| Days Until Release | Release Tasks |
| --- | ---------------- |
| T-2 | * All pending PRs merged in all repos <br> * Begin `cesium-native` release <br> * Update `cesium-native` in all runtimes and perform regression testing |
| T-1 | * Create release commits for `cesium-unity` and `cesium-unreal` <br> * Debug any CI issues * Prepare GitHub release drafts | 
| T-0 | * Publish releases |
