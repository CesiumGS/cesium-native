add_library(Cpp14Compatibility INTERFACE)

cesium_glob_files(CPP14COMPATIBILITY_PUBLIC_HEADERS include/*)

set_target_properties(Cpp14Compatibility
    PROPERTIES
    PUBLIC_HEADER "${CPP14COMPATIBILITY_PUBLIC_HEADERS}"
)

target_include_directories(
  Cpp14Compatibility
  SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(
  Cpp14Compatibility
  INTERFACE
    optional
    mpark_variant
    "string-view-lite"
    "any-lite"
    ghc_filesystem
)
