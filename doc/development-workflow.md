The Cesium Native development workflow allows you to compile Cesium Native on your local system, make changes to it, run the tests, generate documentation, and use the built version in other applications.

Cesium Native uses a number of third-party libraries, and these dependencies are most easily (by far!) installed by using the [Conan Package Manager](https://conan.io/), as described in the instructions below. While the use of Cesium Native without Conan is possible, it is likely to be challenging and is outside the scope of this document.

## Prerequisites

* Visual Studio 2017 (or newer), GCC v7.x+, Clang 10+. Other compilers may work but haven't been tested.
* [CMake](https://cmake.org/) for build automation
* [Conan](https://conan.io/) for dependency management
* [Python 3](https://www.python.org/) for running Conan and the Cesium Native automation tool (`tools/automate.py`)

## Clone the repo

First, clone the repo from GitHub with:

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

Cesium Native is divided into [multiple libraries](../README.md#-libraries-overview), each of which has its own list of dependencies defined in its `library.yml`. We want to use Conan to install these dependencies, but first we need a `conanfile.py` recipe for each library. We can generate the recipes from the yml files by running:

```
./tools/automate.py generate-library-recipes --editable
```

By default, various files for the CMake build are generated in the `build` subdirectory of the cesium-native root. To specify an alternate directory, specify the `--build-folder` option.

> The `--editable` flag to `generate-library-recipes` tells the automation tool that we want recipes for use with editable packages, rather than for use with `conan create`. The primary difference is that editable packages don't list dependencies on _other_ Cesium Native libraries. In the development workflow, dependencies between Cesium Native libraries are managed by CMake rather than Conan.

## Enable Editable Mode

In the normal Conan workflow, Conan builds a library by copying its source code into the Conan cache directory and then performing a clean build. This is a time consuming process, and not what we want in a development workflow.
Instead, we put each Cesium Native library into [editable mode](https://docs.conan.io/en/latest/developing_packages/editable_packages.html), which allows us to use it right in the normal directory where we cloned it from GitHub.

We can enable editable mode for all of the Cesium Native libraries with the following command:

```
./tools/automate.py editable on
```

> You can't create regular Conan packages using `conan create` or `conan export` while the packages are in editable mode. To disable editable mode, run `./tools/automate.py editable off`.

## Install Dependencies

Now that Conan knows about all of the relevent packages, it's time to install (and build, if necessary) all of the dependencies. This is done with a normal `conan install` command. Be sure to specify suitable profiles and build type to the install command. It is also usually necessary to include at least `--build=missing`; `--build=outdated --build=cascade` are recommended. For example, to install all dependencies for both Debug builds with the default profile, use:

```
conan install . --build=outdated --build=cascade -pr:h=default -pr:b=default -s build_type=Debug
```

On CMake multi-configuration systems (Visual Studio, Xcode), it is possible to run this install command multiple times with a different `-s build_type` each time, making it easier to switch between Debug/Release builds. On CMake single-configuration systems like Unix Makefiles, this install step must be repeated when switching between Debug and Release configurations.

## Configure and Build Cesium Native

Now that all the dependencies are in place, we can build Cesium Native using CMake. The `conan install` will generate `build/Cesium[Library]/conan/conan_toolchain.cmake` files that capture some of the settings and options from the Conan profile that was used to install. It's not possible to use a separate toolchain for each library, so we arbitrarily use CesiumUtility's. It shouldn't matter which you use if they're all created by the same `conan install`, because they will have the same settings.

```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/CesiumUtility/conan/conan_toolchain.cmake
```

On single-configuration systems, the toolchain file specifies the `CMAKE_BUILD_TYPE`, so if it that option is also specified on the command-line or elsewhere it will be ignored. Be sure to invoke the `conan install` above for the configuration you want to use.

Note, though, that the toolchain file unfortunately does not capture _every_ setting relevant to the build. For example, if your profile specifies the use of Visual Studio 2017, but CMake on your system defaults to using Clang, you'll get compile or link errors as a result of the dependencies being compiled differently from the Cesium Native libraries. There's no universal solution to this problem, but if it happens to you, you can can probably fix it either by adjusting the [Conan profile](https://docs.conan.io/en/latest/using_packages/using_profiles.html) and reinstalling dependencies, or by specifying a generator or options to CMake. For example, to force CMake to use Visual Studio 2017, you can configure with:

```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/CesiumUtility/conan/conan_toolchain.cmake -G "Visual Studio 15 2017 Win64"
```

In any case, once the configure is complete we can build the Cesium Native libraries and tests with:

```
cmake --build build
```

On multi-configuration systems, as long as we installed the dependencies for all of the required configurations by running `conan install` multiple times above, we can specify the build configuration at build time:

```
cmake --build build --config Debug
cmake --build build --config Release
```

## Generate Documentation

* Install [Doxygen](https://www.doxygen.nl/).
* Run: `cmake --build build --target cesium-native-docs`
* Open `build/doc/html/index.html`
