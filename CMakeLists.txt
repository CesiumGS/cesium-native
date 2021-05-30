cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cesium-native
    VERSION 0.1.0
    LANGUAGES CXX
)

# Add helper modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Add options
option(OPT_BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(OPT_GLOB_USE_CONFIGURE_DEPENDS "Controls if cesium-native targets should use configure_depends or not for globbing their sources" OFF)
option(OPT_BUILD_DOCUMENTATION "Build API documentations of cesium-native" ON)
option(OPT_BUILD_TESTS "Build tests" ON)
option(OPT_BUILD_TESTS_COVERAGE "Build tests coverage" OFF)

# Add postfix "d" to installed debug targets
set(CESIUM_DEBUG_POSTFIX "d")
set(CESIUM_RELEASE_POSTFIX "")
set(CMAKE_DEBUG_POSTFIX ${CESIUM_DEBUG_POSTFIX})
set(CMAKE_RELEASE_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_MINSIZEREL_POSTFIX ${CESIUM_RELEASE_POSTFIX})
set(CMAKE_RELWITHDEBINFO_POSTFIX ${CESIUM_RELEASE_POSTFIX})

# Use configure_depends to automatically reconfigure on filesystem
# changes at the expense of computational overhead for CMake to
# determine if new files have been added (-DGLOB_USE_CONFIGURE_DEPENDS).
function(cesium_glob_files out_var_name regexes)
	if (NOT DEFINED GLOB_USE_CONFIGURE_DEPENDS)
		set(GLOB_USE_CONFIGURE_DEPENDS OFF CACHE BOOL
			"Controls if cesium-native targets should use configure_depends or not for globbing their sources"
		)
	endif()

    set(files "")
    foreach(arg ${ARGV})
        list(APPEND regexes_only "${arg}")
    endforeach()

    list(POP_FRONT regexes_only)
    if (GLOB_USE_CONFIGURE_DEPENDS)
        file(GLOB_RECURSE files CONFIGURE_DEPENDS ${regexes_only})
    else()
        file(GLOB files ${regexes_only})
    endif()

    set(${ARGV0} "${files}" PARENT_SCOPE)
endfunction()

function(configure_cesium_library targetName)
    if (MSVC)
        target_compile_definitions(${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${targetName} PRIVATE /W4 /WX /wd4201)
    else()
        target_compile_options(${targetName} PRIVATE -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow -Wsign-conversion)
    endif()

    set_target_properties(${targetName} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if (OPT_BUILD_SHARED_LIBS)
        target_compile_definitions(
            ${targetName}
            PUBLIC
                CESIUM_SHARED=${OPT_BUILD_SHARED_LIBS}
        )
    endif()

    if (NOT ${targetName} MATCHES "cesium-native-tests")
        string(TOUPPER ${targetName} capitalizedTargetName)
        target_compile_definitions(
            ${targetName}
            PRIVATE
                ${capitalizedTargetName}_BUILDING
        )
    endif()
endfunction()

# Create tests coverage target
if(OPT_BUILD_TESTS_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_html(
        NAME cesium-native-tests-coverage
        EXECUTABLE ctest -j ${PROCESSOR_COUNT}
        EXCLUDE "${PROJECT_SOURCE_DIR}/extern/*" "${PROJECT_BINARY_DIR}"
        DEPENDENCIES cesium-native-tests
    )
endif()

# Add Libraries from Conan
include(ConfigureConan)
find_package(Async++ REQUIRED)
find_package(Catch2 REQUIRED)
find_package(httplib REQUIRED)
find_package(Draco REQUIRED)
find_package(glm REQUIRED)
find_package(magic_enum REQUIRED)
find_package(spdlog REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(stb REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(uriparser REQUIRED)

if (OPT_BUILD_DOCUMENTATION)
    find_package(doxygen REQUIRED)
endif()

# Thirdparty not exists in conan
add_subdirectory(extern)

# Public Targets
add_subdirectory(CesiumUtility)
add_subdirectory(CesiumGltf)
add_subdirectory(CesiumGeometry)
add_subdirectory(CesiumGeospatial)
add_subdirectory(CesiumJsonReader)
add_subdirectory(CesiumGltfWriter)
add_subdirectory(CesiumGltfReader)
add_subdirectory(CesiumAsync)
add_subdirectory(Cesium3DTiles)
add_subdirectory(CesiumIonClient)

# Private Targets
if (OPT_BUILD_TESTS OR OPT_BUILD_TESTS_COVERAGE)
    include(CTest)
	add_subdirectory(CesiumNativeTests)
endif()

if (OPT_BUILD_DOCUMENTATION)
	add_subdirectory(doc)
endif()

# Package third party libraries from conan
include(GNUInstallDirs)
include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)

install(DIRECTORY ${CONAN_LIB_DIRS_ASYNCPLUSPLUS}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_ASYNCPLUSPLUS}/ TYPE INCLUDE)

# cpp-httplib has multiple redundant include directories, so we get only need the first one
list (GET CONAN_INCLUDE_DIRS_CPP-HTTPLIB 0 CONAN_INCLUDE_DIRS_CPP-HTTPLIB) 
install(DIRECTORY ${CONAN_INCLUDE_DIRS_CPP-HTTPLIB}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_DRACO}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_DRACO}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_SPDLOG}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_SPDLOG}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_FMT}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_FMT}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_SQLITE3}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_SQLITE3}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_TINYXML2}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_TINYXML2}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_URIPARSER}/ TYPE LIB)
install(DIRECTORY ${CONAN_INCLUDE_DIRS_URIPARSER}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_INCLUDE_DIRS_GLM}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_INCLUDE_DIRS_MAGIC_ENUM}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_INCLUDE_DIRS_STB}/ TYPE INCLUDE)

# Package third parties not from conan
install(TARGETS modp_b64)

install(TARGETS csprng)

install(TARGETS GSL)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/GSL/include/gsl TYPE INCLUDE)

install(DIRECTORY ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}/rapidjson TYPE INCLUDE)
