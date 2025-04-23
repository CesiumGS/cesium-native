
find_library(zlib-ng_LIBRARIES NAMES zlibstatic-ng z-ng zlib-ng)
find_library(zlib-ng_DEBUG_LIBRARIES
  NAMES
  "zlibstatic-ngd"
  "z-ngd"
  "zlib-ngd"
  )

# vcpkg specific locations for debug libraries if they are not already found
set(zlibngSavePrefixPath ${CMAKE_PREFIX_PATH})
list(FILTER CMAKE_PREFIX_PATH INCLUDE REGEX "/debug")
find_library(zlib-ng_DEBUG_LIBRARIES
  NAMES
  zlibstatic
  z-ng
  zlib-ng
  )
set(CMAKE_PREFIX_PATH ${zlibngSavePrefixPath})

find_path(zlib-ng_INCLUDE_DIRS NAMES zlib-ng.h)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(zlib-ng
  FOUND_VAR
  zlib-ng_FOUND
  REQUIRED_VARS
  zlib-ng_LIBRARIES
  zlib-ng_INCLUDE_DIRS
  )

mark_as_advanced(zlib-ng_LIBRARIES zlib-ng_INCLUDE_DIRS)

if(zlib-ng_FOUND AND NOT TARGET zlib-ng::zlib-ng)
  add_library(zlib-ng::zlib-ng UNKNOWN IMPORTED)
  set_target_properties(zlib-ng::zlib-ng PROPERTIES
    IMPORTED_LOCATION "${zlib-ng_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${zlib-ng_INCLUDE_DIRS}"
    )
  if(zlib-ng_DEBUG_LIBRARIES)
    set_target_properties(zlib-ng::zlib-ng PROPERTIES
      IMPORTED_LOCATION_DEBUG "${zlib-ng_DEBUG_LIBRARIES}"
    )
  endif()
endif()
