# Yes, on non-windows platforms the name of the library really is liblibmodpbase64.a
# Don't ask me, I didn't do it.

find_library(modp_b64_DEBUG_LIBRARIES
  NAMES
  modpbase64d
  libmodpbase64d
  )

# vcpkg specific locations for debug libraries if they are not already found
set(modpbase64SavePrefixPath ${CMAKE_PREFIX_PATH})
list(FILTER CMAKE_PREFIX_PATH INCLUDE REGEX "/debug")
find_library(modp_b64_DEBUG_LIBRARIES
  NAMES
  modpbase64
  libmodpbase64
  )
set(CMAKE_PREFIX_PATH ${modpbase64SavePrefixPath})

set(modpbase64SavePrefixPath ${CMAKE_PREFIX_PATH})
list(FILTER CMAKE_PREFIX_PATH EXCLUDE REGEX "/debug")
find_library(modp_b64_LIBRARIES NAMES modpbase64 libmodpbase64)
set(CMAKE_PREFIX_PATH ${modpbase64SavePrefixPath})

find_path(modp_b64_INCLUDE_DIRS NAMES modp_b64.h)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(modp_b64
  FOUND_VAR
  modp_b64_FOUND
  REQUIRED_VARS
  modp_b64_LIBRARIES
  modp_b64_INCLUDE_DIRS
  )

mark_as_advanced(modp_b64_LIBRARIES modp_b64_INCLUDE_DIRS)

if(modp_b64_FOUND AND NOT TARGET modp_b64::modp_b64)
  add_library(modp_b64::modp_b64 UNKNOWN IMPORTED)
  set_target_properties(modp_b64::modp_b64 PROPERTIES
    IMPORTED_LOCATION "${modp_b64_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${modp_b64_INCLUDE_DIRS}"
    )
  if(modp_b64_DEBUG_LIBRARIES)
    set_target_properties(modp_b64::modp_b64 PROPERTIES
      IMPORTED_LOCATION_DEBUG "${modp_b64_DEBUG_LIBRARIES}"
    )
  endif()
endif()
