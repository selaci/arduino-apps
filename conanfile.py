from conans import ConanFile, CMake, tools

class ArduinoAppsConan(ConanFile):
    name = "arduino_apps"
    version = "0.0.1"
    license = "GPLv3"
    author = "Jordi Grant i Esteve <jgrant.esteve@gmail.com>"
    url = "https://github.com/selaci/arduino_apps"
    description = "Arduino INO files."
    topics = ("arduino", "ino", "apps")

    settings = "os", "compiler", "build_type", "arch"

    options = { "shared": [True, False],
                "fPIC": [True, False],
                "enable_testing": [True, False]}

    default_options = { "shared": False, "fPIC": True, "enable_testing": False }

    generators = "cmake"
    exports = "CMakeLists.txt"
    exports_sources = "src/*"

    requires = "arduino_common/0.0.1", "arduino_gear/0.0.4"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="hello")
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["arduino_apps"]

