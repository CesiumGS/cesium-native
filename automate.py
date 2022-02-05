import argparse
import subprocess
import os

def main():
  argParser = argparse.ArgumentParser(description='Automate cesium-native tasks')

  subparsers = argParser.add_subparsers(dest='task', help='automation command')
  subparsers.required = True

  installDependenciesParser = subparsers.add_parser('install-dependencies', help='install dependencies required to build cesium-native')
  installDependenciesParser.set_defaults(func=installDependencies)
  installDependenciesParser.add_argument("--config", action='append', help='A build configuration for which to install dependencies, such as "Debug", "Release", "MinSizeRel", or "RelWithDebInfo". Can be specified multiple times to install dependencies for multiple configurations. If not specified, dependencies for both "Debug" and "Release" are installed.')

  generateWorkspaceParser = subparsers.add_parser('generate-workspace', help='generate the workspace.cmake, referencing each sub-library')
  generateWorkspaceParser.set_defaults(func=generateWorkspace)

  args = argParser.parse_args()
  args.func(args)

def installDependencies(args):
  libraries = findCesiumLibraries()

  configs = args.config or ['Debug', 'Release']
  print("Installing dependencies for the following build configurations: " + ", ".join(configs))

  # Put all libraries in editable mode
  for library in libraries:
    run("conan editable add %s %s/0.0.0@user/dev" % (library, library))

  # Install dependencies for all libraries in all configurations
  for config in configs:
    for library in libraries:
      run("conan install %s %s/0.0.0@user/dev -if build/%s -s build_type=%s --build missing" % (library, library, library, config))

def generateWorkspace(args):
  libraries = findCesiumLibraries()

  os.makedirs("build", exist_ok=True)
  filename = 'build/workspace.cmake'
  with open(filename, 'w') as f:
    f.writelines(map(lambda library: "add_subdirectory(%s)\n" % (library), libraries))

  print("Workspace file written to " + filename)

def findCesiumLibraries():
  # TODO: Get this from the filesystem, Cesium*
  return [
    "Cesium3DTiles",
    "Cesium3DTilesReader",
    "Cesium3DTilesSelection",
    "Cesium3DTilesWriter",
    "CesiumAsync",
    "CesiumGeometry",
    "CesiumGeospatial",
    "CesiumGltf",
    "CesiumGltfReader",
    "CesiumGltfWriter",
    "CesiumIonClient",
    "CesiumJsonReader",
    "CesiumJsonWriter",
    # "CesiumNativeTests",
    "CesiumUtility",
  ]

def run(command):
  result = subprocess.run(command)
  if result.returncode != 0:
    exit(result.returncode)

main()
