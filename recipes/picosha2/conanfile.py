from conan import ConanFile
from conans.tools import collect_libs
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import patch

class S2GeometryConan(ConanFile):
  name = "picosha2"
  version = "1677374f23352716fc52183255a40c1b8e1d53eb"
  license = "MIT"
  author = "Shintarou Okada (okdshin)"
  url = "https://github.com/okdshin/PicoSHA2"
  description = "A header-file-only, SHA256 hash generator in C++."
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False]}
  default_options = {"shared": False, "fPIC": True}
  generators = "CMakeToolchain", "CMakeDeps"
  requires = [
  ]
  scm = {
    "type": "git",
    "url": "https://github.com/okdshin/PicoSHA2.git",
    "revision": "1677374f23352716fc52183255a40c1b8e1d53eb"
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
