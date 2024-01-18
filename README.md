# Cesium Native

Cesium Native is a set of C++ libraries for 3D geospatial, including:

* [3D Tiles](https://github.com/CesiumGS/3d-tiles) runtime streaming
* lightweight glTF serialization and deserialization, and
* high-precision 3D geospatial math types and functions, including support for global-scale WGS84 ellipsoids.

[![License](https://img.shields.io/:license-Apache_2.0-blue.svg)](https://github.com/CesiumGS/cesium-native/blob/main/LICENSE)
[![Build Status](https://github.com/CesiumGS/cesium-native/actions/workflows/build.yml/badge.svg)](https://github.com/CesiumGS/cesium-native/actions/workflows/build.yml)

Cesium Native powers Cesium's runtime integrations for [Cesium for Unreal](https://github.com/CesiumGS/cesium-unreal), [Cesium for Unity](https://github.com/CesiumGS/cesium-unity), [Cesium for Omniverse](https://github.com/CesiumGS/cesium-omniverse), and [Cesium for O3DE](https://github.com/CesiumGS/cesium-o3de). Cesium Native is the foundational layer for any 3D geospatial software, especially those that want to stream 3D Tiles.

![Cesium Platform and Ecosystem](./doc/integration-ecosystem-diagram.jpg)
*<p align="center">A high-level Cesium platform architecture with the runtime integrations powered by Cesium Native and streaming content from Cesium ion.</p>*

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

* Visual Studio 2019 (or newer), GCC v11.x+, Clang 12+. Other compilers are likely to work but are not regularly tested.
* CMake 3.15+
* For best JPEG-decoding performance, you must have [nasm](https://www.nasm.us/) installed so that CMake can find it. Everything will work fine without it, just slower.

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

#### Compile from command line

```bash
## Windows compilation using Visual Studio
cmake -B build -S . -G "Visual Studio 15 2017 Win64"
cmake --build build --config Debug
cmake --build build --config Release

## Linux compilation
cmake -B build -S .
cmake --build build
```

#### Compile from Visual Studio Code

1) Install the `CMake Tools` extension. It should prompt you to generate project files from CMake.
2) On Windows, choose `Visual Studio 2017 Release - amd64` as the kit to build. Or choose an appropriate kit for your platform.
3) Then press Ctrl-Shift-P and execute the `CMake: Build` task or press F7.

#### Compile with any Visual Studio version using CMake generated projects

1) Open the CMake UI (cmake-gui)
2) Under "Where is the source code", point to your repo
3) Specify your output folder in "Where to build the binaries"
4) Click "Configure".
5) Under "Specify the generator for this project", choose the VS version on your system
6) Click Finish, wait for the process to finish
7) Click Generate

Look for cesium-native.sln in your output folder.

Unit tests can also be run from this solution, under the cesium-native-tests project.

![image](https://github.com/CesiumGS/cesium-native/assets/130494071/4d398bfc-f770-49d4-8ef5-a995096ad4a1)


#### Generate Documentation

* Install [Doxygen](https://www.doxygen.nl/).
* Run: `cmake --build build --target cesium-native-docs`
* Open `build/doc/html/index.html`

#### Regenerate glTF and 3D Tiles classes

Much of the code in `CesiumGltf`, `Cesium3DTiles`, `CesiumGltfReader`, and `Cesium3DTilesReader` is generated from the standards' JSON Schema specifications. To regenerate the code:

* Make sure you have a relatively recent version of Node.js installed.
* Install dependencies by running:

```
npm install
cd tools/generate-classes
npm install
cd ../..
```

* From the repo root directory, run `npm run generate-gltf` or `npm run generate-3d-tiles`.
* On Windows, the line endings of the generated files will be different than those checked into the repo. Just `git add` them and git will fix the line endings (no need to commit).
