from conan import ConanFile
from conans.tools import collect_libs
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import patch

class S2GeometryConan(ConanFile):
  name = "s2geometry"
  version = "0.9.0"
  license = "Apache-2.0"
  author = "Google"
  url = "https://github.com/google/s2geometry"
  description = "Computational geometry and spatial indexing on the sphere"
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False]}
  default_options = {"shared": False, "fPIC": True}
  generators = "CMakeToolchain", "CMakeDeps"
  requires = [
    "openssl/3.0.1",
  ]
  scm = {
    "type": "git",
    "url": "https://github.com/google/s2geometry.git",
    "revision": "v0.9.0"
  }

  exports_sources = ['patches/0001-no-tests.patch']

  def config_options(self):
    if self.settings.os == "Windows":
        del self.options.fPIC

  def generate(self):
    tc = CMakeToolchain(self)
    tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
    tc.variables["BUILD_EXAMPLES"] = False
    tc.generate()

    deps = CMakeDeps(self)
    deps.generate()

  def build(self):
    patch(self, patch_file='patches/0001-no-tests.patch')

    cmake = CMake(self)
    cmake.configure()
    cmake.build()

  def package(self):
    cmake = CMake(self)
    cmake.install()

  def package_info(self):
    self.cpp_info.libs = collect_libs(self)
