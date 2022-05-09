from conan import ConanFile
from conans.tools import collect_libs
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import patch

class S2GeometryConan(ConanFile):
  name = "csprng"
  version = "8768a94b4b04213c0798b80824a04ae4990e9847"
  license = "BSL-1.0"
  author = "Michael Thomas Greer"
  url = "https://github.com/Duthomhas/CSPRNG"
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

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    cmake.build()

  def package(self):
    cmake = CMake(self)
    cmake.install()

  def package_info(self):
    self.cpp_info.libs = collect_libs(self)
