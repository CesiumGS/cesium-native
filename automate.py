import argparse
import subprocess
import os
import yaml

def main():
  argParser = argparse.ArgumentParser(description='Automate cesium-native tasks')

  subparsers = argParser.add_subparsers(dest='task', help='automation command')
  subparsers.required = True

  installDependenciesParser = subparsers.add_parser('install-dependencies', help='install dependencies required to build cesium-native')
  installDependenciesParser.set_defaults(func=installDependencies)
  installDependenciesParser.add_argument('--config', action='append', help='A build configuration for which to install dependencies, such as "Debug", "Release", "MinSizeRel", or "RelWithDebInfo". Can be specified multiple times to install dependencies for multiple configurations. If not specified, dependencies for both "Debug" and "Release" are installed.')

  generateWorkspaceParser = subparsers.add_parser('generate-workspace', help='generate the workspace.cmake, referencing each sub-library')
  generateWorkspaceParser.set_defaults(func=generateWorkspace)

  args = argParser.parse_args()
  args.func(args)

def installDependencies(args):
  libraries = findCesiumLibraries()

  configs = args.config or ['Debug', 'Release']
  print('Installing dependencies for the following build configurations: ' + ', '.join(configs))

  os.makedirs('build', exist_ok=True)

  with open('cesium-native.yml', 'r') as f:
    dependenciesYml = yaml.safe_load(f)

  # Install dependencies for all libraries in all configurations
  for library in libraries:
    with open(library + '/library.yml', 'r') as f:
      libraryYml = yaml.safe_load(f)

    # Generate a conanfile.txt with this library's dependencies
    conanfilename = 'build/conanfile-%s.txt' % (library)
    with open(conanfilename, 'w') as f:
      f.write('[requires]\n')
      dependencies = [*libraryYml['dependencies'], *libraryYml['testDependencies']]
      for dependency in dependencies:
        if dependency in libraries:
          # Don't install other cesium-native library dependencies from Conan
          # because we want to use the version in this repo.
          f.write('# %s\n' % (dependency))
        else:
          f.write('%s/%s\n' % (dependency, dependenciesYml['dependencyVersions'][dependency]))
      f.write('\n')
      f.write('[generators]\n')
      f.write('CMakeDeps\n')

    # Install each build configuration
    for config in configs:
      run('conan install %s -if build/%s/conan -s build_type=%s --build missing' % (conanfilename, library, config))

def generateWorkspace(args):
  libraries = findCesiumLibraries()

  os.makedirs('build', exist_ok=True)
  filename = 'build/workspace.cmake'
  with open(filename, 'w') as f:
    f.writelines(map(lambda library: 'add_subdirectory(%s)\n' % (library), libraries))

  print('Workspace file written to ' + filename)

def findCesiumLibraries():
  # TODO: Get this from the filesystem, Cesium*
  return [
    # 'Cesium3DTiles',
    # 'Cesium3DTilesReader',
    # 'Cesium3DTilesSelection',
    # 'Cesium3DTilesWriter',
    # 'CesiumAsync',
    # 'CesiumGeometry',
    # 'CesiumGeospatial',
    'CesiumGltf',
    # 'CesiumGltfReader',
    # 'CesiumGltfWriter',
    # 'CesiumIonClient',
    # 'CesiumJsonReader',
    # 'CesiumJsonWriter',
    # 'CesiumNativeTests',
    'CesiumUtility',
  ]

def run(command):
  result = subprocess.run(command)
  if result.returncode != 0:
    exit(result.returncode)

main()
