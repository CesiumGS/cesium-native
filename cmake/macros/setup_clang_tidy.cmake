# Additional steps to perform clang-tidy
function(setup_clang_tidy)
    cmake_parse_arguments(
        ""
        ""
        "PROJECT_BUILD_DIRECTORY;ENABLE_CLANG_TIDY_ON_BUILD"
        "PROJECT_SOURCE_DIRECTORIES"
        ${ARGN})

    if(NOT _PROJECT_BUILD_DIRECTORY)
        message(FATAL_ERROR "PROJECT_BUILD_DIRECTORY was not specified")
    endif()

    if(NOT DEFINED _ENABLE_CLANG_TIDY_ON_BUILD)
        message(FATAL_ERROR "ENABLE_CLANG_TIDY_ON_BUILD was not specified")
    endif()

    if(NOT _PROJECT_SOURCE_DIRECTORIES)
        message(FATAL_ERROR "PROJECT_SOURCE_DIRECTORIES was not specified")
    endif()

    # Add clang-tidy
    # our clang-tidy options are located in `.clang-tidy` in the root folder
    # when clang-tidy runs it will look for this file
    if(NOT CLANG_TIDY_PATH)
        get_compiler_tool_with_correct_version(
            TOOL_NAME
            "clang-tidy"
            TOOLCHAIN_NAME
            "Clang"
            RESULT_TOOL_PATH
            CLANG_TIDY_PATH)
    endif()

    # Try to use vswhere to find Visual Studio's installed copy of clang-tidy
    # vswhere should always be in the same location as of Visual Studio 2017 15.2
    set(VSWHERE_PATH "$ENV{ProgramFiles\(x86\)}\\Microsoft Visual Studio\\Installer\\vswhere.exe")
    if(NOT CLANG_TIDY_PATH AND MSVC AND EXISTS ${VSWHERE_PATH})
        execute_process(
            COMMAND
            ${VSWHERE_PATH} -latest -requires Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset -find VC\\Tools\\Llvm\\bin\\clang-tidy.exe
            OUTPUT_VARIABLE CLANG_TIDY_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    if(NOT CLANG_TIDY_PATH)
        message(WARNING "Could not find clang-tidy in your path. clang-tidy targets will not be created")
        return()
    endif()

    message(STATUS "Found clang-tidy: ${CLANG_TIDY_PATH}")

    # Generate compile_commands.json for clang-tidy to use.
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    # compile_commands.json, which clang-tidy requires, is only generated on when CMAKE_GENERATOR is Ninja or Unix Makefiles.
    if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Unix Makefiles")
        # CMake has built-in support for running clang-tidy during the build
        if(_ENABLE_CLANG_TIDY_ON_BUILD)
            if(MSVC)
                # We need to manually tell clang-tidy that exceptions are enabled,
                # as per https://gitlab.kitware.com/cmake/cmake/-/issues/20512#note_722771
                set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH} --extra-arg=/EHsc)
            else()
                # If not set, when building for GCC clang-tidy will pass any warning flags to its underlying clang,
                # causing an error if GCC has a flag that clang does not.
                set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH} --extra-arg=-Wno-unknown-warning-option)
            endif()
            set(CMAKE_CXX_CLANG_TIDY
                ${CMAKE_CXX_CLANG_TIDY}
                PARENT_SCOPE)
        endif()

        # Generate a CMake target that runs clang-tidy by itself
        # `run-clang-tidy` is a python script that comes with llvm that runs clang-tidy in parallel over a compile_commands.json
        # See: https://clang.llvm.org/extra/doxygen/run-clang-tidy_8py_source.html
        if(NOT CLANG_TIDY_RUNNER_PATH)
            get_compiler_tool_with_correct_version(
                TOOL_NAME
                "run-clang-tidy"
                TOOLCHAIN_NAME
                "Clang"
                RESULT_TOOL_PATH
                CLANG_TIDY_RUNNER_PATH)
        endif()

        list(APPEND CLANG_TIDY_RUNNER_PARAMS -extra-arg=-Wno-unknown-warning-option)
        list(APPEND CLANG_TIDY_RUNNER_PARAMS -j${CESIUM_CLANG_TIDY_USE_THREADS})
        list(APPEND CLANG_TIDY_RUNNER_PARAMS -clang-tidy-binary ${CLANG_TIDY_PATH})
        list(APPEND CLANG_TIDY_RUNNER_PARAMS -p ${_PROJECT_BUILD_DIRECTORY}) # path that contains a compile_commands.json
        list(APPEND CLANG_TIDY_RUNNER_PARAMS "\".*(src\/|include\/).*\"")

        if(CLANG_TIDY_RUNNER_PATH)
            message(STATUS "Found run-clang-tidy: ${CLANG_TIDY_RUNNER_PATH}")

            add_custom_target(
                clang-tidy COMMAND ${CLANG_TIDY_RUNNER_PATH} ${CLANG_TIDY_RUNNER_PARAMS}
            )
            add_custom_target(
                clang-tidy-fix COMMAND ${CLANG_TIDY_RUNNER_PATH} -fix ${CLANG_TIDY_RUNNER_PARAMS}
            )
        else()
            message(WARNING "Could not find run-clang-tidy. clang-tidy will still work, but it will run very slowly.")

            # run-clang-tidy was not found, so call clang-tidy directly.
            # this takes a lot longer because it's not parallelized.
            set(SOURCE_EXTENSIONS
                "*.cpp"
                "*.h"
                "*.cxx"
                "*.hxx"
                "*.hpp"
                "*.cc"
                "*.inl")
            file(GLOB directories RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/Cesium*)
            foreach(child_directory ${directories})
                cesium_glob_files(dir_sources
                    ${PROJECT_SOURCE_DIR}/${child_directory}/src/*.cpp
                )
                cesium_glob_files(dir_headers
                    ${PROJECT_SOURCE_DIR}/${child_directory}/src/*.h
                )
                cesium_glob_files(dir_public_headers
                    ${PROJECT_SOURCE_DIR}/${child_directory}/include/CesiumJsonWriter/*.h
                )
                cesium_glob_files(dir_test_sources
                    ${PROJECT_SOURCE_DIR}/${child_directory}/test/*.cpp
                )
                cesium_glob_files(dir_test_headers
                    ${PROJECT_SOURCE_DIR}/${child_directory}/test/*.h
                )
                list(APPEND all_source_files ${dir_sources})
                list(APPEND all_source_files ${dir_headers})
                list(APPEND all_source_files ${dir_public_headers})
                list(APPEND all_source_files ${dir_test_sources})
                list(APPEND all_source_files ${dir_test_headers})
            endforeach()
            add_custom_target(
                clang-tidy
                COMMAND ${CLANG_TIDY_PATH} -p ${_PROJECT_BUILD_DIRECTORY} # path that contains a compile_commands.json
                ${all_source_files})
            add_custom_target(
                clang-tidy-fix
                COMMAND
                ${CLANG_TIDY_PATH} --fix -p
                ${_PROJECT_BUILD_DIRECTORY} # path that contains a compile_commands.json
                ${all_source_files})
        endif()
    endif()
endfunction()
