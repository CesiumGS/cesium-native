# Developer Setup Guide {#developer-setup}

This guide contains the basic setup information for developers looking to work with Cesium Native. **To follow this guide, you will need the following prerequisites:**

* Visual Studio 2019 (or newer), GCC v11.x+, or Clang 12+
  * Other compilers are likely to work but are not regularly tested.
* CMake 3.15+
* For best JPEG-decoding performance, you must have [nasm](https://www.nasm.us/) installed so that CMake can find it. Everything will work fine without it, just slower.
<!--! [TOC] -->

<!--! \cond DOXYGEN_EXCLUDE !-->

<!-- omit in toc -->
## Table of Contents

- [Clone the Repo](#clone-the-repo)
- [Compiling Cesium Native](#compiling-cesium-native)
  - [Compile from command line](#compile-from-command-line)
  - [Compile from Visual Studio Code](#compile-from-visual-studio-code)
  - [Compile with any Visual Studio version using CMake generated projects](#compile-with-any-visual-studio-version-using-cmake-generated-projects)
- [Generate Documentation](#generate-documentation)
- [Regenerate glTF and 3D Tiles classes](#regenerate-gltf-and-3d-tiles-classes)
- [Regenerate Dependency Graphs](#regenerate-dependency-graphs)
- [Run clang-tidy](#run-clang-tidy)

<!--! \endcond -->

## Clone the Repo

Check out the repo with:

```bash
git clone git@github.com:CesiumGS/cesium-native.git
```

## Dependencies

Cesium Native manages its many dependencies using
[vcpkg](https://github.com/microsoft/vcpkg). We use vcpkg's manifest
mode, in which vcpkg reads the list of Cesium Native's dependencies
and installs them. This requires a local installation of `vcpkg` and
some extra arguments to CMake.

## vcpkg

After installing `vcpkg` -- usually by cloning it from
https://github.com/microsoft/vcpkg -- run the proper bootstrap script
and set the `VCPKG_ROOT` environment variable so that Cesium Native's
build can find it:

```bash
cd vcpkg
./bootstrap-vcpkg.sh
export VCPKG_ROOT=$PWD
```

## vcpkg and CMake

`vcpkg` hooks into CMake by loading a toolchain file, which is usually
used in cross-compilation scenarios. You can specify this on the
command line, but this can become a bit verbose as additional `vcpkg` options
get added, so specifying them all in a CMake presets file is an
interesting option. There are example preset files in the
[cmake-presets](../cmake-presets) directory. We haven't included them
in their normal spot in the source tree because they can cause some
issues with VS Code, but you can copy
[CMakeUserPresets.cmake](../cmake-presets/CMakeUserPresets.cmake) to
the root of the Cesium Native source tree to use these predefined
presets.

## Compiling Cesium Native

### Compile from command line

Using presets (on Linux):

```bash
cp doc/cmake-presets/CMakeUserPresets.json .
cmake --preset=vcpkg-linux -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

By hand:
```bash
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DCESIUM_USE_EZVCPG=OFF
```

On Windows:
```bash
cp doc/cmake-presets/CMakeUserPresets.json .
cmake --preset=vcpkg-windows-vs
cmake --build build --config Debug
cmake --build build --config Release
```

`vcpkg` specifies all information about the compilation target in a
"triplet". For compiling Cesium Native as a stand-alone, the system
defaults will usually work, but consumers of Cesium Native might need
something more exotic. For example, one could configure a Windows
build with:

```bash
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DCESIUM_USE_EZVCPG=OFF \
    -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build build --config Debug
cmake --build build --config Release
```


### Compile from Visual Studio Code

1) Install the `CMake Tools` extension. It should prompt you to generate project files from CMake.
2) On Windows, choose `Visual Studio 2017 Release - amd64` as the kit to build. Or choose an appropriate kit for your platform.
3) Then press Ctrl-Shift-P and execute the `CMake: Build` task or press F7.

### Compile with any Visual Studio version using CMake generated projects

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

## Generate Documentation

* Install [Doxygen](https://www.doxygen.nl/).
* Run: `npm install`
* Run: `cmake --build build --target cesium-native-docs`
* Open `build/doc/html/index.html`

## Regenerate glTF and 3D Tiles classes

Much of the code in `CesiumGltf`, `Cesium3DTiles`, `CesiumGltfReader`, `CesiumGltfWriter`, `Cesium3DTilesReader`, `Cesium3DTilesWriter`, and `CesiumQuantizedMeshTerrain` is generated from the standards' JSON Schema specifications. To regenerate the code:

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

## Regenerate Dependency Graphs

The dependency graphs used in the Cesium Native documentation are generated using a script that parses CMake's GraphViz output and generates a set of Mermaid diagrams. To regenerate the graphs:

* Make sure you have a relatively recent version of Node.js installed.
* Install dependencies by running:
```
cd tools/dep-graph-gen
npm install
```
* From the `tools/dep-graph-gen` directory, run `npm run generate-dep-graph` to regenerate the graphs.

## Run clang-tidy

We use [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) to help improve the quality of the code, and an automated process that runs on every commit ensures that there are no clang-tidy warnings or errors. To run it locally, follow the instructions for your system.

### Windows

It's recommended to use [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install) to run clang-tidy on Windows. The following instructions cover using clang-tidy through Visual Studio 2022's CMake support. To set up clang-tidy on WSL2, follow the Linux instructions below.

1. Install clang-tidy for Windows. This can be obtained from [the installer on the LLVM GitHub releases page](https://github.com/llvm/llvm-project/releases). Download `LLVM-19.x.x-win64.exe` and run the installer.

2. clang-tidy requires a `compile_commands.json` file to be generated in the build directory in order to run. This file is only generated with the cmake Ninja or Unix Makefile generators. Visual Studio 2022's CMake support uses the Ninja generator. Open the Cesium Native cmake project with Visual Studio 2022.

3. Right click on the root `CMakeLists.txt` and select "CMake Settings for cesium-native"
![Visual Studio CMake settings](../img/vs-cmake-settings.png)

4. Specify the path to the installed `clang-tidy.exe` under "CMake command arguments":
![Visual Studio CMake path](../img/vs-clang-tidy-path.png)

5. Under "CMake variables and cache", check `CESIUM_ENABLE_CLANG_TIDY_ON_BUILD`:
![CESIUM_ENABLE_CLANG_TIDY_ON_BUILD](../img/vs-clang-tidy-on-build.png)

6. Build cesium-native through Visual Studio 2022 as normal. clang-tidy will be invoked for each build command and its warnings and errors will appear in the build output.


### Linux

1. To get clang-tidy, install llvm 19+. For Debian-based systems, LLVM provides an easy install script:

```
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 19
```
Binaries and source releases for use with other platforms can be downloaded from the [LLVM GitHub releases page](https://github.com/llvm/llvm-project/releases).

2. cmake should detect the installed `clang-tidy-19` automatically, so the project can be configured normally:

```
cmake -B build -S .
```

3. Next, we run the clang-tidy target of Cesium Native's cmake build, which will run clang-tidy. clang-tidy produces a lot of not-very-useful output, so we send it to a file instead of the console:

```
cmake --build build --target clang-tidy > clang-tidy.log
```

4. Finally, we use `sed` to extract the errors and warnings from the log:

```
sed -n '/error:/,/^$/p' clang-tidy.log
sed -n '/warning:/,/^$/p' clang-tidy.log
```


### macOS

1. To get clang-tidy, install llvm 19+. The easiest way is via Homebrew:

```
brew install llvm
```

2. Installing llvm via Homebrew does _not_ add it to your path, so that it doesn't conflict with Xcode. So, we need to specify its location explicitly when running cmake Cesium Native's configure for use with clang-tidy:

```
cmake -B build -S . -DCLANG_TIDY_PATH=$(brew --prefix llvm)/bin/clang-tidy -DCLANG_TIDY_RUNNER_PATH=$(brew --prefix llvm)/bin/run-clang-tidy
```

3. Next, we run the clang-tidy target of Cesium Native's cmake build, which will run clang-tidy. clang-tidy produces a lot of not-very-useful output, so we send it to a file instead of the console:

```
cmake --build build --target clang-tidy > clang-tidy.log
```

4. Finally, we use `sed` to extract the errors and warnings from the log:

```
sed -n '/error:/,/^$/p' clang-tidy.log
sed -n '/warning:/,/^$/p' clang-tidy.log
```
