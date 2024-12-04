<!-- omit in toc -->
# Cesium Native

[![License](https://img.shields.io/:license-Apache_2.0-blue.svg)](https://github.com/CesiumGS/cesium-native/blob/main/LICENSE)
[![Build Status](https://github.com/CesiumGS/cesium-native/actions/workflows/build.yml/badge.svg)](https://github.com/CesiumGS/cesium-native/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/tag/CesiumGS/cesium-native?label=release)](https://github.com/CesiumGS/cesium-native/tags)

<!-- omit in toc -->
## Table of Contents

- [📖About](#about)
  - [🗃️Libraries Overview](#️libraries-overview)
  - [📗License](#license)
  - [🔧Projects Using Cesium Native](#projects-using-cesium-native)
  - [📦Dependencies](#dependencies)
- [💻Developers](#developers)
  - [⭐Prerequisites](#prerequisites)
  - [🚀Getting Started](#getting-started)
    - [Clone the repo](#clone-the-repo)
    - [Compile from command line](#compile-from-command-line)
    - [Compile from Visual Studio Code](#compile-from-visual-studio-code)
    - [Compile with any Visual Studio version using CMake generated projects](#compile-with-any-visual-studio-version-using-cmake-generated-projects)
    - [Generate Documentation](#generate-documentation)
    - [Regenerate glTF and 3D Tiles classes](#regenerate-gltf-and-3d-tiles-classes)
    - [Regenerate Dependency Graphs](#regenerate-dependency-graphs)
- [🤝Contributing](#contributing)
  - [Submitting an Issue](#submitting-an-issue)
  - [Opening a Pull Request](#opening-a-pull-request)
    - [Contributor License Agreement (CLA)](#contributor-license-agreement-cla)
    - [Pull Request Guidelines](#pull-request-guidelines)
  - [Code of Conduct](#code-of-conduct)


## 📖About

Cesium Native is a set of C++ libraries for 3D geospatial, including:

* [3D Tiles](https://github.com/CesiumGS/3d-tiles) runtime streaming
* lightweight glTF serialization and deserialization
* high-precision 3D geospatial math types and functions, including support for global-scale WGS84 ellipsoids.
* support for draping raster overlays from WMS, TMS, WMTS, and other sources over 3D tilesets

Cesium Native powers Cesium's runtime integrations for [Cesium for Unreal](https://github.com/CesiumGS/cesium-unreal), [Cesium for Unity](https://github.com/CesiumGS/cesium-unity), [Cesium for Omniverse](https://github.com/CesiumGS/cesium-omniverse), and [Cesium for O3DE](https://github.com/CesiumGS/cesium-o3de). Cesium Native is the foundational layer for any 3D geospatial software, especially those that want to stream 3D Tiles. See [Projects Using Cesium Native](#projects-using-cesium-native) for a list of projects currently integrating with Cesium Native.

![Cesium Platform and Ecosystem](./doc/integration-ecosystem-diagram.jpg)

<p align="center">*A high-level Cesium platform architecture with the runtime integrations powered by Cesium Native and streaming content from Cesium ion.*</p>

Cesium Native is a collaborative project that welcomes issue reports and pull requests. See the [Contributing](#contributing) section below for more information!

### 🗃️Libraries Overview

| Library                        | Description                                                                                                     |
| ------------------------------ | --------------------------------------------------------------------------------------------------------------- |
| **Cesium3DTiles**              | Lightweight 3D Tiles classes.                                                                                   |
| **Cesium3DTilesContent**       | Classes that support loading and converting 3D Tiles tile content.                                              |
| **Cesium3DTilesReader**        | 3D Tiles deserialization, including 3D Tiles extension support.                                                 |
| **Cesium3DTilesWriter**        | 3D Tiles serialization, including 3D Tiles extension support.                                                   |
| **Cesium3DTilesSelection**     | Runtime streaming, level of detail selection, culling, cache management, and decoding of 3D Tiles.              |
| **CesiumAsync**                | Classes for multi-threaded asynchronous tasks.                                                                  |
| **CesiumGeometry**             | Common 3D geometry classes; and bounds testing, intersection testing, and spatial indexing algorithms.          |
| **CesiumGeospatial**           | 3D geospatial math types and functions for ellipsoids, transforms, projections.                                 |
| **CesiumGltf**                 | Lightweight glTF processing and optimization functions.                                                         |
| **CesiumGltfContent**          | Classes that support manipulating the content of a glTF.                                                        |
| **CesiumGltfReader**           | glTF deserialization / decoding, including glTF extension support (`KHR_draco_mesh_compression` etc).           |
| **CesiumGltfWriter**           | glTF serialization / encoding, including glTF extension support.                                                |
| **CesiumIonClient**            | Functions to access [Cesium ion](https://cesium.com/cesium-ion/) accounts and 3D tilesets using ion's REST API. |
| **CesiumJsonReader**           | Reads JSON from a buffer into statically-typed classes.                                                         |
| **CesiumJsonWriter**           | Writes JSON from statically-typed classes into a buffer.                                                        |
| **CesiumQuantizedMeshTerrain** | Classes for accessing terrain in the [quantized-mesh-1.0](https://github.com/CesiumGS/quantized-mesh) format.   |
| **CesiumRasterOverlays**       | Classes for raster overlays, which allow draping massive 2D textures over a model.                              |
| **CesiumUtility**              | Utility functions for JSON parsing, URI processing, credits, etc.                                               |


### 📗License

[Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0.html). Cesium Native is free for both commercial and non-commercial use.

### 🔧Projects Using Cesium Native

The following official Cesium integrations are built on top of Cesium Native:

- [Cesium for Unreal](https://github.com/CesiumGS/cesium-unreal)
- [Cesium for Unity](https://github.com/CesiumGS/cesium-unity)
- [Cesium for Omniverse](https://github.com/CesiumGS/cesium-omniverse)
- [Cesium for O3DE](https://github.com/CesiumGS/cesium-o3de)

In addition, Cesium Native has been used by the community to build projects including:
- [vsgCs](https://github.com/timoore/vsgCs) integrates Cesium Native with VulkanSceneGraph.
- [osgEarth](https://github.com/gwaldron/osgearth) uses Cesium Native to load 3D Tiles in OpenSceneGraph.

If you have a project that integrates with Cesium Native that isn't on this list yet, please let us know!

### 📦Dependencies

Cesium Native relies on various third-party packages to accomplish tasks like image decoding, URI parsing, and math operations. These dependencies are automatically obtained and built using [vcpkg](https://vcpkg.io/en/) when building through CMake. Below is a graph of all the Cesium Native libraries and their dependencies on third party packages and each other.

![](doc/img/dependency-graph.svg)

## 💻Developers

### ⭐Prerequisites

* Visual Studio 2019 (or newer), GCC v11.x+, Clang 12+. Other compilers are likely to work but are not regularly tested.
* CMake 3.15+
* For best JPEG-decoding performance, you must have [nasm](https://www.nasm.us/) installed so that CMake can find it. Everything will work fine without it, just slower.

### 🚀Getting Started

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
* Run: `npm install`
* Run: `cmake --build build --target cesium-native-docs`
* Open `build/doc/html/index.html`

#### Regenerate glTF and 3D Tiles classes

Much of the code in `CesiumGltf`, `Cesium3DTiles`, `CesiumGltfReader`, `Cesium3DTilesReader`, and `CesiumQuantizedMeshTerrain` is generated from the standards' JSON Schema specifications. To regenerate the code:

* Make sure you have a relatively recent version of Node.js installed.
* Install dependencies by running:

```
npm install
cd tools/generate-classes
npm install
cd ../..
```

* From the repo root directory, run these commands
  * `npm run generate-gltf`
  * `npm run generate-3d-tiles`
  * `npm run generate-quantized-mesh-terrain`
* On Windows, the line endings of the generated files will be different than those checked into the repo. Just `git add` them and git will fix the line endings (no need to commit).

#### Regenerate Dependency Graphs

The dependency graphs used in the Cesium Native documentation are generated using a script that parses CMake's GraphViz output and generates a set of Mermaid diagrams. To regenerate the graphs:

* Make sure you have a relatively recent version of Node.js installed.
* Install dependencies by running:
```
cd tools/dep-graph-gen
npm install
```
* From the `tools/dep-graph-gen` directory, run `npm run generate-dep-graph` to regenerate the graphs.

## 🤝Contributing

Community involvement in Cesium Native has been and continues to be a key part of its development. Contributing to Cesium Native can take many forms:

- Making a pull request to add features or fix issues.
- Submitting an issue to report a bug.
- Triaging existing issues. This can include attempting to reproduce issues or sharing additional information you have.
- Being active on the [Cesium community forum](https://community.cesium.com/) by answering questions and providing input on Cesium's direction.
- Writing tutorials, creating examples, and improving the reference documentation.
- Sharing projects you've made with Cesium Native with us through the [Cesium community forum](https://community.cesium.com/) or at hello@cesium.com.

More details about writing issues and pull requests is detailed below. For any other guidance you need, don't hesitate to ask on the [Cesium community forum](https://community.cesium.com/)!

### Submitting an Issue

If you have a question, please do not submit an issue; instead, search the [Cesium community forum](https://community.cesium.com/). The forum is very active and there are years of informative archives, often with answers from the core Cesium team. If you do not find an answer to your question, start a new thread and you'll likely get a quick response!

If you think you've found a bug in Cesium Native, first search the [issues](https://github.com/CesiumGS/cesium-native/issues). If an issue already exists, please add a comment expressing your interest and any additional information. This helps us prioritize issues.

If a related issue does not exist, submit a new one. Please be concise and include as much of the following information as is relevant:

- Minimum amount of sample code (and data).
- Screenshot or animated .gif if appropriate (try [LICEcap](http://www.cockos.com/licecap/)). For example, see [#803](https://github.com/CesiumGS/cesium-native/issues/803). Screenshots are particularly useful for exceptions and rendering artifacts.
- Link to the thread if this was discussed on the Cesium forum or elsewhere. For example, see [#878](https://github.com/CesiumGS/cesium-native/issues/878).
- Your operating system and version, compiler and version, and video card. If you're using Cesium Native with an engine like Unreal or Unity, please include this information as well. Are they all up-to-date? Is the issue specific to one of them?
- The version of Cesium Native. Did this work in a previous version?
- Ideas for how to fix or workaround the issue. Also mention if you are willing to help fix it. If so, the Cesium team can often provide guidance and the issue may get fixed more quickly with your help.

### Opening a Pull Request

Pull requests are a huge help in the development of Cesium Native. Following the tips in this guide will help your pull request get merged quickly.

> If you plan to make a major change, please start a new thread on the [Cesium community forum](https://community.cesium.com/) first. Pull requests for small features and bug fixes can generally just be opened without discussion on the forum.

#### Contributor License Agreement (CLA)

Before we can review a pull request, we require a signed Contributor License Agreement. There is a CLA for:

- [individuals](https://docs.google.com/forms/d/e/1FAIpQLScU-yvQdcdjCFHkNXwdNeEXx5Qhu45QXuWX_uF5qiLGFSEwlA/viewform) and
- [corporations](https://docs.google.com/forms/d/e/1FAIpQLSeYEaWlBl1tQEiegfHMuqnH9VxyfgXGyIw13C2sN7Fj3J3GVA/viewform).

This only needs to be completed once, and enables contributions to all of the projects under the [CesiumGS](https://github.com/CesiumGS) organization, including Cesium Native. The CLA ensures you retain copyright to your contributions, and provides us the right to use, modify, and redistribute your contributions using the [Apache 2.0 License](LICENSE.md).

If you have any questions, feel free to reach out to [hello@cesium.com](mailto:hello@cesium)!

#### Pull Request Guidelines

Our code is our lifeblood so maintaining Cesium Native's high code quality is important to us.
- For an overview of our workflow see [github pull request workflows](https://cesium.com/blog/2013/10/08/github-pull-request-workflows/).
- Pull request tips
  - If your pull request fixes an existing issue, include a link to the issue in the description (like this: "Fixes [#1](https://github.com/CesiumGS/cesium-native/issues/1)"). Likewise, if your pull request fixes an issue reported on the Cesium forum, include a link to the thread.
  - If your pull request needs additional work, include a [task list](https://github.com/blog/1375%0A-task-lists-in-gfm-issues-pulls-comments).
  - Once you are done making new commits to address feedback, add a comment to the pull request such as `"this is ready"` since GitHub doesn't notify us about commits.
- Code and tests
  - Review the [C++ Style Guide](doc/topics/style-guide.md). These guidelines help us write consistent, performant, less buggy code and improve our productivity by standardizing the decisions we make across the codebase.
  - Verify that all tests pass, and write new tests with excellent code coverage for new code. Tests can be built and run using the `cesium-native-tests` target.
  - Update [CHANGES.md](CHANGES.md) with a brief description of your changes.
  - If you plan to add a third-party library, start a [GitHub issue](https://github.com/CesiumGS/cesium/issues/new) discussing it first.

### Code of Conduct

To ensure an inclusive community, contributors and users in the Cesium community should follow the [code of conduct](https://github.com/CesiumGS/cesium/blob/main/CODE_OF_CONDUCT.md).