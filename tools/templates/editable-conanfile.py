from conan import ConanFile
from conans import tools
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
import os

class {{name}}Conan(ConanFile):
    name = "{{conanName}}"
    version = "{{native.version}}"
    {% if native.user %}
    user = "{{native.user}}"
    {% endif %}
    {% if native.user %}
    channel = "{{native.channel}}"
    {% endif %}
    license = "Apache-2.0"
    author = "CesiumGS, Inc. and Contributors"
    url = "https://github.com/CesiumGS/cesium-native"
    description = "{{library.description}}"
    topics = () # TODO
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    requires = [
      {% for lib in dependencies %}
      "{{lib}}",
      {% endfor %}
      {% for lib in testDependencies %}
      "{{lib}}",
      {% endfor %}
    ]

    exports_sources = [
      "include/*",
      "generated/*",
      "src/*",
      "test/*",
      "CMakeLists.txt",
      "../tools/cmake/cesium.cmake"
    ]

    def config_options(self):
      if self.settings.os == "Windows":
          del self.options.fPIC

    def generate(self):
      tc = CMakeToolchain(self)
      # In editable mode, don't reference other cesium-native libraries via Conan
      tc.variables["CESIUM_USE_CONAN_PACKAGES"] = False
      tc.variables["CESIUM_TESTS_ENABLED"] = False # TODO
      tc.generate()

      deps = CMakeDeps(self)
      deps.generate()

    def build(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.build()
      # if self.develop:
      #   cmake.test()

    def package(self):
      cmake = CMake(self)
      cmake.install()

    def package_info(self):
      self.cpp_info.libs = ["{{name}}"]
      self.cpp_info.set_property("cmake_file_name", "{{name}}")
      self.cpp_info.set_property("cmake_target_name", "{{name}}::{{name}}")

    def layout(self):
      self.folders.source = "."
      self.folders.build = os.path.join("..", "{{buildFolder}}", "{{name}}")
      self.folders.generators = self.folders.build + "/conan"
      self.cpp.source.includedirs = ["include", "generated/include"]
      self.cpp.source.libdirs = [self.folders.build + "/Debug"]
