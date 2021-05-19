# cmake-conan configuration

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

# Add the list of dependencies for the project
# When adding a new one add a comment on the side stating why is needed
conan_cmake_configure(REQUIRES 
                        asyncplusplus/1.1
                        catch2/2.13.6
                        cpp-httplib/0.8.8
                        draco/1.3.6
                        glm/0.9.9.8
                        magic_enum/0.7.2
                        spdlog/1.8.5 
                        sqlite3/3.35.5
                        stb/20200203
                        tinyxml2/8.0.0
                        uriparser/0.9.5
                        doxygen/1.9.1
					GENERATORS 
                        cmake_find_package
					OPTIONS 
                        spdlog:no_exceptions=True)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conan-center
                    SETTINGS ${settings})

