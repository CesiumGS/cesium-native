add_subdirectory(GSL)

add_subdirectory(modp_b64)

# CSPRNG's CMake setup is old school, and it really only has one source
# file, so set up a library manually here.
add_library(csprng CSPRNG/source/csprng.cpp)
target_include_directories(
    csprng
    PUBLIC
        CSPRNG/source
)
set(CSPRNG_HEADERS 
        ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.h 
        ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/csprng.hpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/CSPRNG/source/duthomhas/is_iterable.hpp)

# PicoSHA2 doesn't have CMakeLists.txt at all
add_library(PicoSHA2 INTERFACE)
target_include_directories(PicoSHA2 SYSTEM INTERFACE PicoSHA2)
set_target_properties(PicoSHA2 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/PicoSHA2/picosha2.h")

set(CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/rapidjson/include" CACHE INTERNAL
    "Include directory for rapidjson"
)

set(CESIUM_NATIVE_RAPIDJSON_DEFINES
    RAPIDJSON_HAS_CXX11_RVALUE_REFS
    CACHE INTERNAL
    "Compiler definitions for rapidjson"
)

