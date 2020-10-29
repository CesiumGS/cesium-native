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



# Function to collect all targets of the current directory 
# and all subdirectories (https://stackoverflow.com/a/62311397)
function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()

    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
endmacro()

# Ingnore "warning C4661: '...': no suitable definition provided for explicit template instantiation request"
# for all draco targets
get_all_targets(all_targets)
foreach(target ${all_targets}) 
    if ("${target}" MATCHES "draco.*")
        if (MSVC)
            target_compile_options(${target} PUBLIC /wd4661)
        endif()
    endif()
endforeach(target)

