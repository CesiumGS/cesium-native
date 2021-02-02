add_subdirectory(Catch2)

add_subdirectory(GSL)

option(URIPARSER_BUILD_TESTS off)
option(URIPARSER_BUILD_DOCS off)
option(URIPARSER_ENABLE_INSTALL off)
option(URIPARSER_BUILD_TOOLS off)
add_subdirectory(uriparser)

add_subdirectory(draco)

add_subdirectory(glm)

set(BUILD_TESTS OFF CACHE BOOL "Build tinyxml2 tests" FORCE)
add_subdirectory(tinyxml2)

add_subdirectory(asyncplusplus)

add_subdirectory(spdlog)

add_subdirectory(sqlite3)

add_subdirectory(modp_b64)
