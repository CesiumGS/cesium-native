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
option(OPT_BUILD_TESTS "Build tests" ON)
option(OPT_BUILD_TESTS_COVERAGE "Build tests coverage" OFF)
option(OPT_PRIVATE_CESIUM_SQLITE "ON to rename SQLite symbols to cesium_sqlite3_* so they won't conflict with other SQLite implemenentations" OFF)

# Change the default build type to Release. Conan will require CMAKE_BUILD_TYPE
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type of build: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

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
find_package(stb REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(uriparser REQUIRED)
find_package(RapidJSON REQUIRED)

# Thirdparty not exists in conan
add_subdirectory(extern)

# Cesium Native Targets
include(ConfigureTarget)
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
add_subdirectory(doc)

if (OPT_BUILD_TESTS OR OPT_BUILD_TESTS_COVERAGE)
    include(CTest)
	add_subdirectory(CesiumNativeTests)
endif()

# package third party libraries from conan
include(GNUInstallDirs)
include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)

install(DIRECTORY ${CONAN_LIB_DIRS_ASYNCPLUSPLUS}/ 
		TYPE LIB 
		FILES_MATCHING 
		PATTERN "*.a"
		PATTERN "*.lib")
install(DIRECTORY ${CONAN_INCLUDE_DIRS_ASYNCPLUSPLUS}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_SPDLOG}/ 
		TYPE LIB
		FILES_MATCHING 
		PATTERN "*.a"
		PATTERN "*.lib")
install(DIRECTORY ${CONAN_INCLUDE_DIRS_SPDLOG}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_FMT}/ 
		TYPE LIB
		FILES_MATCHING 
		PATTERN "*.a"
		PATTERN "*.lib")
install(DIRECTORY ${CONAN_INCLUDE_DIRS_FMT}/ TYPE INCLUDE)

install(DIRECTORY ${CONAN_LIB_DIRS_DRACO}/ 
		TYPE LIB
		FILES_MATCHING 
		PATTERN "*.a"
		PATTERN "*.lib")

install(DIRECTORY ${CONAN_LIB_DIRS_TINYXML2}/ 
		TYPE LIB
		FILES_MATCHING 
		PATTERN "*.a"
		PATTERN "*.lib")

install(DIRECTORY ${CONAN_LIB_DIRS_URIPARSER}/ 
		TYPE LIB
		FILES_MATCHING 
		PATTERN "*.a"
		PATTERN "*.lib")

install(DIRECTORY ${CONAN_INCLUDE_DIRS_GLM}/ TYPE INCLUDE)

install(FILES ${CONAN_INCLUDE_DIRS_STB}/stb_image_resize.h TYPE INCLUDE)

install(DIRECTORY ${CONAN_INCLUDE_DIRS_RAPIDJSON}/ TYPE INCLUDE)

# Package third parties not from conan
install(TARGETS modp_b64)

install(TARGETS csprng)

install(TARGETS GSL)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/GSL/include/gsl TYPE INCLUDE)

install(TARGETS sqlite3)