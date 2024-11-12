# if (MSVC)
#     add_compile_options(/W4 /WX /wd4201 /bigobj)
# else()
#     add_compile_options(-Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
# endif()

include("cmake/macros/get_compiler_tool.cmake")

# Additional steps to perform clang-tidy
function(setup_linters)
    cmake_parse_arguments(
        ""
        ""
        "PROJECT_BUILD_DIRECTORY;ENABLE_LINTERS_ON_BUILD"
        "PROJECT_SOURCE_DIRECTORIES"
        ${ARGN})

    if(NOT _PROJECT_BUILD_DIRECTORY)
        message(FATAL_ERROR "PROJECT_BUILD_DIRECTORY was not specified")
    endif()

    if(NOT DEFINED _ENABLE_LINTERS_ON_BUILD)
        message(FATAL_ERROR "ENABLE_LINTERS_ON_BUILD was not specified")
    endif()

    if(NOT _PROJECT_SOURCE_DIRECTORIES)
        message(FATAL_ERROR "PROJECT_SOURCE_DIRECTORIES was not specified")
    endif()

      # Add clang-tidy
      # our clang-tidy options are located in `.clang-tidy` in the root folder
      # when clang-tidy runs it will look for this file
      get_compiler_tool_with_correct_version(
          TOOL_NAME
          "clang-tidy"
          TOOLCHAIN_NAME
          "Clang"
          RESULT_TOOL_PATH
          CLANG_TIDY_PATH)

      if(NOT CLANG_TIDY_PATH)
          message(FATAL_ERROR "Could not find clang-tidy in your path.")
      endif()

      # CMake has built-in support for running clang-tidy during the build
      if(_ENABLE_LINTERS_ON_BUILD)
          set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH})
          set(CMAKE_CXX_CLANG_TIDY
              ${CMAKE_CXX_CLANG_TIDY}
              PARENT_SCOPE)
      endif()

      # Generate a CMake target that runs clang-tidy by itself
      # `run-clang-tidy` is a python script that comes with llvm that runs clang-tidy in parallel over a compile_commands.json
      # See: https://clang.llvm.org/extra/doxygen/run-clang-tidy_8py_source.html
      get_compiler_tool_with_correct_version(
          TOOL_NAME
          "run-clang-tidy"
          TOOLCHAIN_NAME
          "Clang"
          RESULT_TOOL_PATH
          CLANG_TIDY_RUNNER_PATH)

      if(CLANG_TIDY_RUNNER_PATH)
          add_custom_target(
              clang-tidy COMMAND ${CLANG_TIDY_RUNNER_PATH} -clang-tidy-binary ${CLANG_TIDY_PATH} -p
                                  ${_PROJECT_BUILD_DIRECTORY} # path that contains a compile_commands.json
          )
          add_custom_target(
              clang-tidy-fix COMMAND ${CLANG_TIDY_RUNNER_PATH} -fix -clang-tidy-binary ${CLANG_TIDY_PATH} -p
                                      ${_PROJECT_BUILD_DIRECTORY} # path that contains a compile_commands.json
          )
      else()
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
          foreach(source_directory ${PROJECT_SOURCE_DIRECTORIES})
              foreach(source_extension ${SOURCE_EXTENSIONS})
                  file(GLOB_RECURSE source_directory_files ${source_directory}/${source_extension})
                  list(APPEND all_source_files ${source_directory_files})
              endforeach()
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
endfunction()
