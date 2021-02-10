set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)

add_subdirectory(Catch2)
set(CATCH2_SINGLE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/Catch2/single_include" CACHE INTERNAL
    "Include directory for single.hpp version of Catch2"
)

add_subdirectory(GSL)
option(URIPARSER_BUILD_TESTS off)
option(URIPARSER_BUILD_DOCS off)
option(URIPARSER_ENABLE_INSTALL off)
option(URIPARSER_BUILD_TOOLS off)
add_subdirectory(uriparser)
if (NOT TARGET draco)
    add_subdirectory(draco)
endif()
add_subdirectory(glm)
set(BUILD_TESTS OFF CACHE BOOL "Build tinyxml2 tests" FORCE)
add_subdirectory(tinyxml2)
add_subdirectory(asyncplusplus)
add_subdirectory(spdlog)
add_subdirectory(modp_b64)
add_subdirectory(sqlite3)

set(CESIUM_NATIVE_MAGIC_ENUM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/magic_enum/" CACHE INTERNAL 
    "Include directory for magic_enum"
)

set(CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/rapidjson/include" CACHE INTERNAL
    "Include directory for rapidjson"
)

set(CESIUM_NATIVE_DRACO_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/draco/src/" CACHE INTERNAL
    "Include directory for Draco"
)

set(CESIUM_NATIVE_GLM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/glm" CACHE INTERNAL
    "Include directory for GLM"
)

set(CESIUM_NATIVE_STB_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/stb" CACHE INTERNAL
    "Include directory for STB libraries"

set(MAGIC_ENUM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/magic_enum/" CACHE INTERNAL "")