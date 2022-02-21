from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

class CesiumNativeConan(ConanFile):
    name = "cesium-native"
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
    description = "Top-level package that depends on all the other cesium-native library packages"
    topics = () # TODO
    settings = "os", "compiler", "build_type", "arch"
    options = []
    requires = [
      {% for lib in dependencies %}
      "{{lib}}",
      {% endfor %}
    ]

    def generate(self):
      tc = CMakeToolchain(self)
      tc.variables["CESIUM_USE_CONAN_PACKAGES"] = {{'False' if skipNativeDependencies else 'True'}}
      tc.variables["CESIUM_TESTS_ENABLED"] = True
      tc.generate()

    def layout(self):
      self.folders.source = "."
      self.folders.build = "{{buildFolder}}"
      self.folders.generators = self.folders.build + "/conan"
      self.cpp.source.includedirs = []
      self.cpp.source.libdirs = []
