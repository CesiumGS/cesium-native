project(sqlite3)

add_library(sqlite3 STATIC sqlite3.c)
set_target_properties(sqlite3 PROPERTIES COMPILE_DEFINITIONS SQLITE_OMIT_LOAD_EXTENSION)
set_target_properties(sqlite3 PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.h")
target_compile_definitions(sqlite3 PUBLIC SQLITE_THREADSAFE=1)

