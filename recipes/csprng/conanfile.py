from conan import ConanFile
from conans.tools import collect_libs
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import patch

class S2GeometryConan(ConanFile):
  name = "csprng"
  version = "8768a94b4b04213c0798b80824a04ae4990e9847"
  license = "BSL-1.0"
  author = "Michael Thomas Greer"
  url = "https://github.com/Duthomhas/CSPRNG.git"
  description = "A small C & C++ library to use the OS's Crytpgraphically Secure Pseudo-Random Number Generator"
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False]}
  default_options = {"shared": False, "fPIC": True}
  generators = "CMakeToolchain", "CMakeDeps"
  requires = [
  ]
  scm = {
    "type": "git",
    "url": "https://github.com/Duthomhas/CSPRNG.git",
    "revision": "8768a94b4b04213c0798b80824a04ae4990e9847"
  }

  # The library's CMakeLists.txt isn't usable, so supply our own.
  exports_sources = ['CMakeLists.txt']

  def config_options(self):
    if self.settings.os == "Windows":
        del self.options.fPIC

  # def generate(self):
  #   tc = CMakeToolchain(self)
  #   tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
  #   tc.variables["BUILD_EXAMPLES"] = False
  #   tc.generate()

  #   deps = CMakeDeps(self)
  #   deps.generate()

  def build(self):
    # patch(self, patch_file='patches/0001-no-tests.patch')

    cmake = CMake(self)
    cmake.configure()
    cmake.build()

  def package(self):
    cmake = CMake(self)
    cmake.install()

  def package_info(self):
    self.cpp_info.libs = collect_libs(self)
