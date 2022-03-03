This package was copied from https://github.com/conan-io/conan-center-index/tree/master/recipes/ktx/all and has the following changes:

* Modified to use a new commit (a7159924a6095488d8fb1ded358113d85813f669) rather than the v4.0.0 tag.
* Applies a patch (patches/0004-no-debug-defines.patch) to not define `_DEBUG`, which causes problems when Debug builds use the Release C runtime library.
* Adds a version attribute to conanfile.py so that the recipe can be exported without specifying a version.
