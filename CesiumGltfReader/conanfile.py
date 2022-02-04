from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout

class CesiumGltfReaderConan(ConanFile):
    name = "CesiumGltfReader"
    version = "0.12.0"
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
      "base64/0.4.0",
      "draco/1.4.3",
      "ms-gsl/4.0.0",
      "stb/cci.20210713"
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
      "CesiumAsync",
      "CesiumGltf",
      "CesiumJsonReader",
      "CesiumUtility"
    ]

    def requirements(self):
      # For other cesium-native packages, use the same version, user, and channel.
      for lib in self.cesiumNativeDependencies:
        try:
          user = self.user
          channel = self.channel
        except ConanException:
            self.requires("%s/%s" % (lib, self.version))
        else:
            self.requires("%s/%s@%s/%s" % (lib, self.version, user, channel))

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

    def layout(self):
      # Mostly a default cmake layout
      cmake_layout(self)

      # But build into the top-level build directory
      self.folders.build = "../" + self.folders.build + "/" + self.name
      self.folders.generators = self.folders.build + "/conan"

      # And includes aren't in src
      self.cpp.source.includedirs = ["include"]
