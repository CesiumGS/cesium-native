project(sqlite3)

add_library(sqlite3 STATIC "")
set_target_properties(sqlite3 PROPERTIES COMPILE_DEFINITIONS SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(sqlite3 PUBLIC SQLITE_THREADSAFE=1)

# Optionally rename all of the public symbols in sqlite3 so they don't conflict with Unreal Engine's version of sqlite.
# We do this by renaming sqlite3* to cesium_sqlite3* in the sqlite amalgamation source and header files.
# We also #define PRIVATE_CESIUM_SQLITE to cue clients to use the new names.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.h SQLITE3_HEADER_CONTENTS)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c SQLITE3_SOURCE_CONTENTS)

if (PRIVATE_CESIUM_SQLITE)
    string(REPLACE sqlite3 cesium_sqlite3 SQLITE3_HEADER_CONTENTS "${SQLITE3_HEADER_CONTENTS}")
    string(REPLACE sqlite3 cesium_sqlite3 SQLITE3_SOURCE_CONTENTS "${SQLITE3_SOURCE_CONTENTS}")
    target_compile_definitions(sqlite3 PUBLIC PRIVATE_CESIUM_SQLITE)
endif()

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/sqlite3.h CONTENT "${SQLITE3_HEADER_CONTENTS}")
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/sqlite3.c CONTENT "${SQLITE3_SOURCE_CONTENTS}")
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/cesium-sqlite3.h INPUT ${CMAKE_CURRENT_SOURCE_DIR}/cesium-sqlite3.h)

target_sources(
    sqlite3
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/sqlite3/sqlite3.c
)

target_include_directories(
    sqlite3
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/sqlite3
)

install(TARGETS sqlite3
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sqlite3
)
