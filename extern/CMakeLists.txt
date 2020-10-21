add_subdirectory(Catch2)

add_subdirectory(GSL)

option(URIPARSER_BUILD_TESTS off)
option(URIPARSER_BUILD_DOCS off)
option(URIPARSER_ENABLE_INSTALL off)
option(URIPARSER_BUILD_TOOLS off)
add_subdirectory(uriparser)

add_subdirectory(draco)

add_subdirectory(glm)

set(BUILD_TESTS OFF)
add_subdirectory(tinyxml2)
