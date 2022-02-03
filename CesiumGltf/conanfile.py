from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake
from conan.tools.layout import cmake_layout

class CesiumGltfConan(ConanFile):
    name = "CesiumGltf"
    version = "0.12.0"
    # user = None
    # channel = None
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of CesiumGltf here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps"
    requires = [
      "ms-gsl/3.1.0"
    ]

    exports_sources = [
      "generated/*",
      "include/*",
      "src/*",
      "test/*",
      "CMakeLists.txt",
      "../tools/cmake/cesium.cmake"
    ]

    cesiumNativeDependencies = [
      "CesiumUtility"
    ]

    def requirements(self):
      print("*********** " + self.recipe_folder)
      print("************ CesiumGltf requirements")
      for lib in self.cesiumNativeDependencies:
        if self.user and self.channel:
          s = "%s/%s@%s/%s" % (lib, self.version, self.user, self.channel)
          print("********* " + s)
          self.requires(s)
        else:
          s = "%s/%s" % (lib, self.version)
          print("********* " + s)
          self.requires(s)

    def config_options(self):
      print("************ CesiumGltf config_options")
      if self.settings.os == "Windows":
          del self.options.fPIC

    def build(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.build()

    def package(self):
      cmake = CMake(self)
      cmake.install()
