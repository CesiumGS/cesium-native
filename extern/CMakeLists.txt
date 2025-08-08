# The bulk of the external dependencies are now automatically downlaoded and built using vcpkg.

if (WIN32)
  set(EXTERN_LIB_SUFFIX "lib")
  set(EXTERN_LIB_PREFIX "")
else()
  set(EXTERN_LIB_SUFFIX "a")
  set(EXTERN_LIB_PREFIX "lib")
endif()

add_library(include_helpers INTERFACE)
target_include_directories(include_helpers INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")

add_library(earcut INTERFACE)
find_path(EARCUT_HPP_INCLUDE_DIRS "mapbox/earcut.hpp")
target_include_directories(earcut INTERFACE "${EARCUT_HPP_INCLUDE_DIRS}")

# picosha2 also doesn't contain a CMake config, so we construct a fake header-only import library here
add_library(picosha2::picosha2 IMPORTED INTERFACE GLOBAL)
set_target_properties(picosha2::picosha2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_BUILD_DIR}/include"
)
