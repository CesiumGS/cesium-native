set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
add_subdirectory(Catch2)

add_subdirectory(GSL)

option(URIPARSER_BUILD_TESTS "" off)
option(URIPARSER_BUILD_DOCS "" off)
option(URIPARSER_ENABLE_INSTALL "" on)
option(URIPARSER_BUILD_TOOLS "" off)
add_subdirectory(uriparser)
install(TARGETS uriparser OPTIONAL) # Skips headers

if (NOT TARGET draco)
    add_subdirectory(draco)
endif()

if (NOT TARGET glm)
    add_subdirectory(glm GLM)
    install(DIRECTORY glm/glm
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      COMPONENT GLM
    )
endif()

set(BUILD_TESTS OFF CACHE BOOL "Build tinyxml2 tests" FORCE)
add_subdirectory(tinyxml2)
install(TARGETS tinyxml2)

add_subdirectory(asyncplusplus)
install(TARGETS Async++)

set(SPDLOG_BUILD_TESTING OFF CACHE INTERNAL "Disable SPDLOG Testing")
add_subdirectory(spdlog)
install(TARGETS spdlog)

if (NOT TARGET sqlite3)
  add_subdirectory(sqlite3)

  set_target_properties(sqlite3 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.h")

  install(TARGETS sqlite3)
endif()

add_subdirectory(modp_b64)
set_target_properties(modp_b64 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/modp_b64/modp_b64.h")
install(TARGETS modp_b64)

add_subdirectory(cpp-httplib)
install(TARGETS httplib)

# CSPRNG's CMake setup is old school, and it really only has one source
# file, so set up a library manually here.
add_library(csprng CSPRNG/source/csprng.cpp)
target_include_directories(
    csprng
    PUBLIC
        CSPRNG/source
)
set(CSPRNG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.h ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.hpp ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/is_iterable.hpp)
set_target_properties(csprng PROPERTIES PUBLIC_HEADER "${CSPRNG_HEADERS}")
install(TARGETS csprng
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/duthomhas/)

# PicoSHA2 doesn't have CMakeLists.txt at all
add_library(PicoSHA2 INTERFACE)
target_include_directories(PicoSHA2 SYSTEM INTERFACE PicoSHA2)
set_target_properties(PicoSHA2 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/PicoSHA2/picosha2.h")
install(TARGETS PicoSHA2)

set(CESIUM_NATIVE_SPDLOG_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/spdlog/include" CACHE INTERNAL
    "Include directory for spdlog"
)

install(DIRECTORY ${CESIUM_NATIVE_SPDLOG_INCLUDE_DIR}/spdlog DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

set(CESIUM_NATIVE_SQLITE3_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/sqlite3/" CACHE INTERNAL
    "Include directory for sqlite3"
)

set(CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/rapidjson/include" CACHE INTERNAL
    "Include directory for rapidjson"
)

install(DIRECTORY ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}/rapidjson DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

set(CESIUM_NATIVE_RAPIDJSON_DEFINES
    RAPIDJSON_HAS_CXX11_RVALUE_REFS
    CACHE INTERNAL
    "Compiler definitions for rapidjson"
)

set(CESIUM_NATIVE_DRACO_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/draco/src/" CACHE INTERNAL
    "Include directory for Draco"
)

set(CESIUM_NATIVE_STB_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/stb" CACHE INTERNAL
    "Include directory for STB libraries"
)
