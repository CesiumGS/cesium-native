from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake
from conan.tools.layout import cmake_layout

class CesiumUtilityConan(ConanFile):
    name = "CesiumUtility"
    version = "0.12.0"
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of CesiumUtility here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps"
    requires = [
      "ms-gsl/3.1.0",
      "glm/0.9.9.8",
      "uriparser/0.9.6",
      "rapidjson/cci.20211112"
    ]

    exports_sources = [
      "include/*",
      "src/*",
      "test/*",
      "CMakeLists.txt",
      "../tools/cmake/cesium.cmake"
    ]

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
