#!/usr/bin/env python3
import argparse
import subprocess
import os
import yaml
import jinja2
from enum import Enum

def main():
  argParser = argparse.ArgumentParser(description='Automate cesium-native tasks')

  subparsers = argParser.add_subparsers(dest='task', help='automation command')
  subparsers.required = True

  installDependenciesParser = subparsers.add_parser('install-dependencies', help='install dependencies required to build cesium-native')
  installDependenciesParser.set_defaults(func=installDependencies)
  installDependenciesParser.add_argument('--config', action='append', help='A build configuration for which to install dependencies, such as "Debug", "Release", "MinSizeRel", or "RelWithDebInfo". Can be specified multiple times to install dependencies for multiple configurations. If not specified, dependencies for both "Debug" and "Release" are installed.')
  installDependenciesParser.add_argument('-pr:h', dest='hostProfile', help='The Conan host profile to use', default='default')
  installDependenciesParser.add_argument('-pr:b', dest='buildProfile', help='The Conan build profile to use', default='default')

  generateWorkspaceParser = subparsers.add_parser('generate-workspace', help='generate the workspace.cmake, referencing each sub-library')
  generateWorkspaceParser.set_defaults(func=generateWorkspace)

  generateWorkspaceParser = subparsers.add_parser('create-recipes', help='create Conan recipes to package cesium-native libraries')
  generateWorkspaceParser.set_defaults(func=createRecipes)

  listDependenciesParser = subparsers.add_parser('list-dependencies', help='list the cesium-native libraries in dependency order, with libraries depended-upon listed before those that depend on them')
  listDependenciesParser.set_defaults(func=lambda x: executeInDependencyOrder(findCesiumLibraries(), lambda library: print(library)))

  createPackagesParser = subparsers.add_parser('create-packages', help='create a Conan package (conan create) for each cesium-native library')
  createPackagesParser.add_argument('--config', help='The build configuration for which to create packages, such as "Debug", "Release", "MinSizeRel", or "RelWithDebInfo".', default='Debug')
  createPackagesParser.add_argument('-pr:h', dest='hostProfile', help='The Conan host profile to use', default='default')
  createPackagesParser.add_argument('-pr:b', dest='buildProfile', help='The Conan build profile to use', default='default')
  createPackagesParser.set_defaults(func=createPackages)

  args = argParser.parse_args()
  args.func(args)

def installDependencies(args):
  libraries = findCesiumLibraries()

  configs = args.config or ['Release', 'Debug']
  print('Installing dependencies for the following build configurations: ' + ', '.join(configs))

  os.makedirs('build', exist_ok=True)

  with open('cesium-native.yml', 'r') as f:
    nativeYml = yaml.safe_load(f)

  # Create packages from custom recipes
  for recipe in nativeYml['extraRecipes']:
    for config in configs:
      run('conan create recipes/%s -pr:h=%s -pr:b=%s -s build_type=%s' % (recipe, args.hostProfile, args.buildProfile, config))

  # Install dependencies for all libraries in all configurations
  for library in libraries:
    with open(library + '/library.yml', 'r') as f:
      libraryYml = yaml.safe_load(f)

    # Generate a conanfile.txt with this library's dependencies
    conanfilename = 'build/conanfile-%s.txt' % (library)
    with open(conanfilename, 'w') as f:
      f.write('[requires]\n')

      # Combine the regular and test dependencies.
      # Filter out dependencies on other cesium-native libraries because we want to
      # use the version this repo.
      dependencies = filter(lambda item: not item in libraries, [*libraryYml['dependencies'], *libraryYml['testDependencies']])

      # Resolve the version of each dependency, and write it.
      resolvedDependencies = resolveDependencies(libraries, nativeYml, dependencies)
      f.writelines(map(lambda x: x + '\n', resolvedDependencies))

      f.write('\n')
      f.write('[generators]\n')
      f.write('CMakeDeps\n')

    # Install each build configuration
    for config in configs:
      run('conan install %s -if build/%s/conan -pr:h=%s -pr:b=%s -s build_type=%s --build missing' % (conanfilename, library, args.hostProfile, args.buildProfile, config))

  # Generate a toolchain file to make CMake match the Conan settings.
  # In multi-config environments (e.g. Visual Studio), we need to do this for each build configuration.
  # In single-config environments (e.g. Makefiles), the last one wins but overriding the build config
  # by passing `-DCMAKE_BUILD_TYPE=Release` will still work except that the generated
  # build/conan/conan_toolchain.cmake hardcodes CMAKE_BUILD_TYPE. Comment out that line and you're good.
  os.makedirs('build/conan', exist_ok=True)
  with open('build/conan/conanfile.txt', 'w') as f:
    f.write('[requires]\n\n')
    f.write('[generators]\n')
    f.write('CMakeToolchain')

  for config in configs:
    run('conan install build/conan/conanfile.txt -if build/conan -pr:h=%s -pr:b=%s -s build_type=%s' % (args.hostProfile, args.buildProfile, config))

def generateWorkspace(args):
  libraries = findCesiumLibraries()

  os.makedirs('build', exist_ok=True)
  filename = 'build/workspace.cmake'
  with open(filename, 'w') as f:
    f.writelines(map(lambda library: 'add_subdirectory(%s)\n' % (library), libraries))

  print('Workspace file written to ' + filename)

def createRecipes(args):
  env = jinja2.Environment(
      loader=jinja2.PackageLoader("automate"),
      autoescape=jinja2.select_autoescape(),
      trim_blocks=True,
      lstrip_blocks=True
  )

  template = env.get_template("conanfile.py")

  libraries = findCesiumLibraries()

  with open('cesium-native.yml', 'r') as f:
    nativeYml = yaml.safe_load(f)

  # Write a recipe for every library.
  for library in libraries:
    with open(library + '/library.yml', 'r') as f:
      libraryYml = yaml.safe_load(f)

      path = '%s' % (library)
      os.makedirs(path, exist_ok=True)
      with open(path + '/conanfile.py', 'w') as f:
        f.write('# Do not edit this file! It is created by running "tools/automate.py create-recipes"\n\n')
        f.write(template.render(
          name=library,
          native=nativeYml,
          library=libraryYml,
          dependencies=resolveDependencies(libraries, nativeYml, libraryYml['dependencies']),
          testDependencies=resolveDependencies(libraries, nativeYml, libraryYml['testDependencies'])
        ))

def findCesiumLibraries():
  # TODO: Get this from the filesystem, Cesium*
  return [
    'Cesium3DTiles',
    'Cesium3DTilesReader',
    'Cesium3DTilesSelection',
    'Cesium3DTilesWriter',
    'CesiumAsync',
    'CesiumGeometry',
    'CesiumGeospatial',
    'CesiumGltf',
    'CesiumGltfReader',
    'CesiumGltfWriter',
    'CesiumIonClient',
    'CesiumJsonReader',
    'CesiumJsonWriter',
    'CesiumUtility',
  ]

def resolveDependencies(nativeLibraries, nativeYml, dependencies):
  def resolveDependency(library):
    if library in nativeLibraries:
      if nativeYml['user'] or nativeYml['channel']:
        return '%s/%s@%s/%s' % (library, nativeYml['version'], nativeYml['user'], nativeYml['channel'])
      else:
        return '%s/%s' % (library, nativeYml['version'])
    else:
      versions = nativeYml['dependencyVersions']
      if library in versions:
        return '%s/%s' % (library, versions[library])
      else:
        return '%s/specify.version.in.cesium-native.yml' % (library)

  return map(resolveDependency, dependencies)

def executeInDependencyOrder(nativeLibraries, func):
  class State(Enum):
    NOT_VISITED = 0
    VISITED_DOWN = 1
    VISITED_UP = 2

  states = {}

  # Initially all libraries are not visited
  for library in nativeLibraries:
    states[library] = State.NOT_VISITED

  def recurse(library):
    currentState = states[library]
    if currentState == State.VISITED_DOWN:
      print('Detected a cycle in the dependency graph while visiting %s!' % (library))
      return

    if currentState == State.VISITED_UP:
      # Already visited
      return

    states[library] = State.VISITED_DOWN

    with open(library + '/library.yml', 'r') as f:
      libraryYml = yaml.safe_load(f)

    dependencies = [*libraryYml['dependencies'], *libraryYml['testDependencies']]
    for dependency in dependencies:
      if dependency.startswith('Cesium'):
        recurse(dependency)

    states[library] = State.VISITED_UP

    func(library)

  for library in nativeLibraries:
    recurse(library)

def createPackages(args):
  def createPackage(library):
    run('conan create %s -pr:h=%s -pr:b=%s -s build_type=%s' % (library, args.hostProfile, args.buildProfile, args.config))

  nativeLibraries = findCesiumLibraries()
  executeInDependencyOrder(nativeLibraries, createPackage)

def run(command):
  result = subprocess.run(command, shell=True)
  if result.returncode != 0:
    exit(result.returncode)

main()
