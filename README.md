# Cesium Native

Cesium Native is a set of C++ libraries for 3D geospatial, including:

* [3D Tiles](https://github.com/CesiumGS/3d-tiles) runtime streaming
* lightweight glTF serialization and deserialization, and
* high-precision 3D geospatial math types and functions, including support for global-scale WGS84 ellipsoids.

[![License](https://img.shields.io/:license-Apache_2.0-blue.svg)](https://github.com/CesiumGS/cesium-native/blob/main/LICENSE)
[![Build Status](https://api.travis-ci.com/CesiumGS/cesium-native.svg?token=z6LPvn37d5E37hGcTgua&branch=main&status=passed)](https://travis-ci.com/CesiumGS/cesium-native)

Currently Cesium Native is used to develop [Cesium for Unreal](https://github.com/CesiumGS/cesium-unreal) and [Cesium for O3DE](https://github.com/CesiumGS/cesium-o3de). In the future, we plan for Cesium Native to be a foundational layer for any 3D geospatial software, especially those that want to stream 3D Tiles.

![Cesium for Unreal Architecture](./doc/unreal-architecture.png)
*<p align="center">A high-level architecture of Cesium for Unreal, Cesium Native and Unreal Engine streaming content from Cesium ion.</p>*


### :card_file_box:Libraries Overview

| Library | Description |
| -- | -- |
| **Cesium3DTiles** | Lightweight 3D Tiles classes. |
| **Cesium3DTilesReader** | 3D Tiles deserialization, including 3D Tiles extension support. |
| **Cesium3DTilesWriter** | 3D Tiles serialization, including 3D Tiles extension support. |
| **Cesium3DTilesSelection** | Runtime streaming, decoding, level of detail selection, culling, cache management, and decoding of 3D Tiles. |
| **CesiumAsync** | Classes for multi-threaded asynchronous tasks. |
| **CesiumGeometry** | Common 3D geometry classes; and bounds testing, intersection testing, and spatial indexing algorithms. |
| **CesiumGeospatial** | 3D geospatial math types and functions for ellipsoids, transforms, projections. |
| **CesiumGltf** | Lightweight glTF processing and optimization functions. |
| **CesiumGltfReader** | glTF deserialization / decoding, including glTF extension support (`KHR_draco_mesh_compression` etc). |
| **CesiumGltfWriter** | glTF serialization / encoding, including glTF extension support. |
| **CesiumIonClient** | Functions to access [Cesium ion](https://cesium.com/cesium-ion/) accounts and 3D tilesets using ion's REST API. |
| **CesiumJsonReader** | Reads JSON from a buffer into statically-typed classes. |
| **CesiumJsonWriter** | Writes JSON from statically-typed classes into a buffer. |
| **CesiumUtility** | Utility functions for JSON parsing, URI processing, etc. |


### :green_book:License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0.html). Cesium Native is free for both commercial and non-commercial use.

## 💻Developers

### ⭐Prerequisites

* Visual Studio 2017 (or newer), GCC v7.x+, Clang 10+. Other compilers may work but haven't been tested.
* [CMake](https://cmake.org/)
* [Conan](https://conan.io/)

### :rocket:Getting Started

#### Clone the repo

Check out the repo with:

```bash
git clone git@github.com:CesiumGS/cesium-native.git
```

#### Add third-party packages to the Conan cache

Cesium Native requires a few third-party packages that are not (yet) on Conan Center. These need to be added to the local Conan cache prior to any of the steps below. You can manually `conan export` each subdirectory of `recipes/`, or run an automation command to do them all in one go:

```
./tools/automate.py conan-export-recipes
```

This generally only needs to be done once, but may need to happen again if Cesium Native is later updated with a new or changed third-party library. It's quick and harmless to run multiple times.

### Create Conan Packages

If you're not planning to change Cesium Native itself, but just want to create some Conan packages in your Conan cache that you can upload to a package repository or use from another Conan-enabled application, that's easy to do. First, you'll need a recipe (conanfile.py) for each Cesium Native library, suitable for this purpose. To generate those, run:

```
./tools/automate.py generate-library-recipes
```

This will generate a `conanfile.py` in each `Cesium*` library directory. Note that the version, user, and channel of the packages is controlled by the values in `cesium-native.yml`. The requirements (dependencies) of each library are specified in the `library.yml` file in each library directory.

Next, export the Cesium Native libraries to the Conan cache. This won't create any binaries for them yet:

```
./tools/automate.py conan-export-libraries
```

Now you can build any Cesium Native library using the normal `conan create` command in the root directory. Because no binaries exist yet for many of the libraries, it is necessary to a use at least the `--build=missing` option. For example, the following command creates a Release package for all of the Cesium Native libraries using the default host and build profiles:

```
conan create . --build=missing --build=outdated -pr:h=default -pr:b=default -s build_type=Release
```

You can also create a package for a single Ceisum Native library (and the packages it depends on) by running the `conan create` in the library's subdirectory:

```
conan create CesiumUtility --build=missing --build=outdated -pr:h=default -pr:b=default -s build_type=Release
```

#### Development Workflow

If you do plan to modify Cesium Native, the above can be time consuming because every build in the Conan cache is a clean build from scratch. In development, it's helpful to be able to iterate more quickly. We can do that with Conan's "editables" feature. First, enable editable mode for every Cesium Native library:

```
./tools/automate.py editable on
```

(Note: You can't create regular Conan packages using `conan create` or `conan export` while the packages are in editable mode. To disable editable mode, run `./tools/automate.py editable off`.)

Next, we need to make sure that built binaries for the third-party libraries required by Cesium Native are in the Conan cache, and that Cesium Native knows how to find them. To that end, we need a Conan recipe for each Cesium library. These recipes are different from the ones used above in that they only list third-party dependencies. During development, dependencies among Cesium Native libraries are handled internally by CMake. To generate these `conanfile.py` files, run:

```
./tools/automate.py generate-library-recipes --editable
```

By default, various files for the CMake build are generated in the `build` subdirectory of the cesium-native root. To specify an alternate directory, specify the `--build-folder` option.

Then we need to do a `conan install` to actually download or build all of the dependencies. Be sure to specify suitable profiles and build type to the install command. It is also usually necessary to include at least `--build=missing`. For example, to install all dependencies for a debug build with the default profile, use:

```
conan install . --build=missing --build=outdated -pr:h=default -pr:b=default -s build_type=Release
```

(Note the symmetry with the `conan create` command in the previous section; it is not a coincidence!)

It is possible to run this install command multiple times with a different `-s build_type` each time, making it easier to switch between Debug/Release builds. This is true even on CMake single-configuration systems like Unix Makefiles.

Finally, we build Cesium Native using CMake. Be sure to match the Conan profile settings and options (which control how the third-party library binaries are generated) with the CMake configuration (which controls how the Cesium Native libraries are built). It can be helpful to use a Conan CMake toolchain file generated in the install step above, but not every setting is captured in the toolchain so conflicts are still possible. Configure Cesium Native's CMake build with:

```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
```

And build it with:

```
cmake --build build
```

As long as the dependencies don't change, we can simply invoke the CMake build above (and configure when necessary) for a quick development cycle.

#### Compile

You can then build cesium-native on the command-line with CMake:

```bash
## Windows compilation using Visual Studio
cmake -B build -S . -G "Visual Studio 15 2017 Win64"
cmake --build build --config Debug
cmake --build build --config Release

## Linux compilation
cmake -B build -S .
cmake --build build
```

Or, you can easily build it in Visual Studio Code with the `CMake Tools` extension installed. It should prompt you to generate project files from CMake. On Windows, choose `Visual Studio 2017 Release - amd64` as the kit to build. Or choose an appropriate kit for your platform. Then press Ctrl-Shift-P and execute the `CMake: Build` task or press F7.

#### Generate Documentation

* Install [Doxygen](https://www.doxygen.nl/).
* Run: `cmake --build build --target cesium-native-docs`
* Open `build/doc/html/index.html`
