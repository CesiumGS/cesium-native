project(sqlite3)

add_library(sqlite3 STATIC sqlite3.c sqlite3.h sqlite3ext.h)
set_target_properties(sqlite3 PROPERTIES COMPILE_DEFINITIONS SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(sqlite3 PUBLIC SQLITE_THREADSAFE=2)
target_include_directories(sqlite3 PUBLIC sqlite3.h)

add_executable(sqlite3_shell shell.c sqlite3.h sqlite3ext.h)
set_target_properties(sqlite3_shell PROPERTIES COMPILE_DEFINITIONS SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(sqlite3_shell PUBLIC SQLITE_THREADSAFE=2)
add_dependencies(sqlite3_shell sqlite3)
target_link_libraries(sqlite3_shell sqlite3)