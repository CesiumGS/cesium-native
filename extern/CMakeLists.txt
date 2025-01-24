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



# Modp_b64 doesn't contain a CMake config, so we construct a fake import library here
add_library(modp_b64::modp_b64 STATIC IMPORTED GLOBAL)
set_target_properties(modp_b64::modp_b64 PROPERTIES
    INCLUDE_DIRECTORIES "${PACKAGE_BUILD_DIR}/include"
    # Yes, on non-windows platforms the name of the library really is liblibmodpbase64.a
    # Don't ask me, I didn't do it.
    IMPORTED_LOCATION "${PACKAGE_BUILD_DIR}/lib/${EXTERN_LIB_PREFIX}libmodpbase64.${EXTERN_LIB_SUFFIX}"
    IMPORTED_LOCATION_DEBUG "${PACKAGE_BUILD_DIR}/debug/lib/${EXTERN_LIB_PREFIX}libmodpbase64.${EXTERN_LIB_SUFFIX}"
)

# picosha2 also doesn't contain a CMake config, so we construct a fake header-only import library here
add_library(picosha2::picosha2 IMPORTED INTERFACE GLOBAL)
set_target_properties(picosha2::picosha2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_BUILD_DIR}/include"
)

find_library(ZLIB_NG_RELEASE_LIB NAMES zlibstatic-ng z-ng PATHS ${PACKAGE_BUILD_DIR}/lib NO_DEFAULT_PATH)
find_library(ZLIB_NG_DEBUG_LIB NAMES zlibstatic-ngd z-ng PATHS ${PACKAGE_BUILD_DIR}/debug/lib NO_DEFAULT_PATH)
add_library(zlib-ng::zlib-ng STATIC IMPORTED GLOBAL)
target_include_directories(zlib-ng::zlib-ng INTERFACE "${PACKAGE_BUILD_DIR}/include")
set_target_properties(zlib-ng::zlib-ng PROPERTIES
  IMPORTED_LOCATION "${ZLIB_NG_RELEASE_LIB}"
  IMPORTED_LOCATION_DEBUG "${ZLIB_NG_DEBUG_LIB}"
)
