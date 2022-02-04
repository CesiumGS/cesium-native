import argparse
import subprocess

def main():
  argParser = argparse.ArgumentParser(description='Automate cesium-native tasks')

  subparsers = argParser.add_subparsers(dest='task', help='automation command')
  subparsers.required = True

  installDependenciesParser = subparsers.add_parser('install-dependencies', help='install dependencies required to build cesium-native')
  installDependenciesParser.set_defaults(func=installDependencies)
  installDependenciesParser.add_argument("--config", action='append')

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
    run("conan editable add %s %s/0.12.0@user/dev" % (library, library))

  # Install dependencies for all libraries, and for all configs
  for config in configs:
    for library in libraries:
      run("conan install %s %s/0.12.0@user/dev -if build/%s -s build_type=%s --build missing" % (library, library, library, config))

def generateWorkspace(args):
  libraries = findCesiumLibraries()

  filename = 'build/workspace.cmake'
  with open(filename, 'w') as f:
    f.writelines(map(lambda library: "add_subdirectory(%s)\n" % (library), libraries))

  print("Workspace file written to " + filename)

def findCesiumLibraries():
  # TODO: Get this from the filesystem, Cesium*
  return [
    "CesiumUtility",
    "CesiumGltf",
    "CesiumGeometry",
    "CesiumGeospatial",
    "CesiumAsync",
    "CesiumJsonReader",
    "CesiumJsonWriter",
    "CesiumGltfReader"
  ]

def run(command):
  result = subprocess.run(command)
  if result.returncode != 0:
    exit(result.returncode)

main()
