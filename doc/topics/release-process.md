# Releasing a new version of Cesium Native {#release-process}

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
