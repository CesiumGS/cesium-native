# Cesium Native

Cesium Native is the geospatial runtime engine powering [Cesium for Unreal](https://github.com/CesiumGS/cesium-unreal).

Cesium Native is a set of C++ libraries for [3D Tiles](https://github.com/CesiumGS/3d-tiles) runtime, glTF, and high-precision 3D geospatial math to enable high-precision full-scale WGS84 virtual globes.

[![License](https://img.shields.io/:license-Apache_2.0-blue.svg)](https://github.com/CesiumGS/cesium-native/blob/main/LICENSE)
[![Build Status](https://api.travis-ci.com/CesiumGS/cesium-native.svg?token=z6LPvn37d5E37hGcTgua&branch=main&status=passed)](https://travis-ci.com/CesiumGS/cesium-native)

### :card_file_box: Libraries Overview

| Library | Description |
| -- | -- |
| **Cesium3DTiles** | Runtime streaming, traversal, and decoding of 3D Tiles. |
| **CesiumAsync** | Library to perform multi-threaded asynchronous tasks. |
| **CesiumGeometry** | 3D Geometry library for common 3D geometry, bounds and intersection testing, and spatial indexing. |
| **CesiumGeospatial** | Library for 3D geospatial math such as ellipsoids, transforms, projections. |
| **CesiumGltf** | glTF processing library. |
| **CesiumGltfReader** | Library for glTF 2.0 parsing, decoding, and processing. Supports optimizations such as Draco. |
| **CesiumIonClient** | Functionality to integrate with [Cesium ion](https://cesium.com/cesium-ion) using REST API. |
| **CesiumUtility** | Library for utility and helper functions for JSON, URI, etc. |

### :books: API Documentation

TODO: Add link to https://cesium.com/docs

### :card_index: Roadmap

The roadmap of Cesium Native is closely linked with the roadmap of [Cesium for Unreal](https://github.com/CesiumGS/cesium-unreal). TODO: Link to Cesium for Unreal roadmap.

### :green_book: License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0.html). Cesium for Unreal is free for both commercial and non-commercial use.

## 💻Developers

### ⭐Prerequisites

* Visual Studio 2017 (or 2019) or GCC v7.x+. Other compilers may work but haven't been tested.
* CMake

### :rocket:Getting Started

#### Clone the repo

Check out the repo with:

```bash
git clone git@github.com:CesiumGS/cesium-native.git --recurse-submodules
```

If you forget the `--recurse-submodules`, nothing will work because the git submodules will be missing. You should be able to fix it with:

```bash
git submodule update --init --recursive
```

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

* Install Doxygen.
* Run: `cmake --build build --target cesium-native-docs`
* Open `build/doc/html/index.html`
