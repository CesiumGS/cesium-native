project(sqlite3)

message(${CMAKE_CURRENT_LIST_DIR})
add_library(sqlite3 STATIC sqlite3.c)
set_target_properties(sqlite3 PROPERTIES COMPILE_DEFINITIONS SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(sqlite3 PUBLIC SQLITE_THREADSAFE=1)
target_include_directories(sqlite3 PUBLIC sqlite3.h)

