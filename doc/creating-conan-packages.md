If you're not planning to change Cesium Native itself, but just want to create some Conan packages in your Conan cache that you can upload to a package repository or use from another Conan-enabled application, that's easy to do by following these instructions.

## Prerequisites

* Visual Studio 2017 (or newer), GCC v7.x+, Clang 10+. Other compilers may work but haven't been tested.
* [CMake](https://cmake.org/) for build automation
* [Conan](https://conan.io/) for dependency management
* [Python 3](https://www.python.org/) for running Conan and the Cesium Native automation tool (`tools/automate.py`)

## Clone the repo

First, check out the repo from GitHub with:

```bash
git clone https://github.com/CesiumGS/cesium-native.git
```

## Add third-party packages to the Conan cache

Cesium Native requires a few third-party packages that are not (yet) on Conan Center. These need to be added to the local Conan cache prior to any of the steps below. You can manually `conan export` each subdirectory of `recipes/`, or run an automation command to do them all in one go:

```
./tools/automate.py conan-export-recipes
```

This generally only needs to be done once, but may need to happen again if Cesium Native is later updated with a new or changed third-party library. It's quick and harmless to run multiple times.

## Generate Recipes

Cesium Native is divided into [multiple libraries](../README.md#libraries), each of which has its own list of dependencies and other informationed defined in its `library.yml`. The `cesium-native.yml` file in the root directory contains settings common to all of the Cesium Native libraries, including the version, user, and channel of the packages to be created. To create a package for each of the Cesium Native libraries, we first need a `conanfile.py` recipe for each library. We can generate the recipes by running:

```
./tools/automate.py generate-library-recipes
```

## Disable Editable Mode

If you previously used the [Development Workflow](development-workflow.md), the Cesium Native packages are currently in [editable mode](https://docs.conan.io/en/latest/developing_packages/editable_packages.html), which doesn't allow package creation. To disable editable mode, run:

```
./tools/automate.py editable off
```

## Export Packages to the Conan Cache

Next, export the Cesium Native libraries to the Conan cache:

```
./tools/automate.py conan-export-libraries
```

This simply invokes `conan export` for each Cesium Native library, adding the recipe and source code (but not any binaries yet) to the Conan cache.

## Create Packages

With all of the library packages exported to the Conan cache, we can now create a binary package for any of them with a normal `conan create` on any library's directory. Because no binaries exist yet for many of the libraries, It is also usually necessary to include at least the `--build=missing` option; `--build=outdated --build=cascade` are recommended. For example, the following command creates a Release package for `CesiumGltf` using the default host and build profiles:

```
conan create CesiumGltf --build=outdated --build=cascade -pr:h=default -pr:b=default -s build_type=Release
```

You can create packages for _all_ Cesium Native libraries by running the `conan create` in the root directory:

```
conan create . --build=outdated --build=cascade -pr:h=default -pr:b=default -s build_type=Release
```
