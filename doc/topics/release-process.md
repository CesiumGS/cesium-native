# Releasing a new version of Cesium Native {#native-release-process}

This is the process we follow when releasing a new version of Cesium Native.

## Prepare for Release

Run the `/prepare-release` skill in the [GitHub-hosted skill file](https://github.com/CesiumGS/cesium-native/tree/main/.github/skills/prepare-release/SKILL.md) and provide the release version number. You can do this in the GitHub Copilot chat by typing `/prepare-release <version-number>`. It performs these four steps for you:

1. Verify that CI is passing on all platforms. Fix it if not.
2. Verify that `CHANGES.md` is complete and accurate.
   - Give the header of the section containing the latest changes an appropriate version number and date.
   - Diff main against the previous released version. This helps catch changes that are missing from the changelog, as well as changelog entries that were accidentally added to the wrong section.
3. Set the `version` property in `package.json`.
4. Set the `VERSION` property passed to the `project()` function in `CMakeLists.txt`.

After the prompt has completed, review and commit these changes. You can push them directly to `main`.

## Release

Use the release helper script to tag the release, push the tag to GitHub, and fast-forward the [cesium.com](https://github.com/CesiumGS/cesium-native/tree/cesium.com) branch to the new tag. Replace the version number with the one used in the preparation steps above.

```bash
npm run publish-release -- 0.56.0
```

Use `--dry-run` to preview the commands without changing anything, `--yes` to skip the confirmation prompt, or `--remote <name>` to target a non-default remote.

This script performs the release and docs publication steps in one operation:

1. Creates and pushes the `v0.56.0` tag.
2. Checks out `cesium.com` and fast-forwards it to the tag.
3. Pushes the updated `cesium.com` branch.
4. Returns to `main`.


# Release Schedule

## Release Coordinator

| Date      | User             |
| --------- | ---------------- |
| 6/1/2026  | `@timoore`       |
| 7/1/2026  | `@azrogers`      |
| 8/3/2026  | `@j9liu`         |
| 9/1/2026  | `@timoore`       |
| 10/1/2026 | `@azrogers`      |
| 11/2/2026 | `@j9liu`         |
| 12/1/2026 | `@timoore`       |

## Cesium Native Runtimes Release Timeline

| Days Until Release | Release Tasks |
| --- | ---------------- |
| T-2 | * All pending PRs merged in all repos <br> * Begin `cesium-native` release <br> * Update `cesium-native` in all runtimes and perform regression testing |
| T-1 | * Create release commits for `cesium-unity` and `cesium-unreal` <br> * Debug any CI issues * Prepare GitHub release drafts | 
| T-0 | * Publish releases |
