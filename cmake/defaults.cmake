# Default values for all projects that can be set before the project() command
# No `if()` statements allowed in here, particularly anything that's predecated
# on platform or compiler detection

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

file(GLOB CUSTOM_MACROS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros/*.cmake")
foreach(CUSTOM_MACRO ${CUSTOM_MACROS})
  include(${CUSTOM_MACRO})
endforeach()
