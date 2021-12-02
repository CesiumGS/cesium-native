add_library(CesiumAsync "")

configure_cesium_library(CesiumAsync)

cesium_glob_files(CESIUM_ASYNC_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_ASYNC_HEADERS src/*.h)
cesium_glob_files(CESIUM_ASYNC_PUBLIC_HEADERS include/CesiumAsync/*.h)
cesium_glob_files(CESIUM_ASYNC_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_ASYNC_TEST_HEADERS test/*.h)

set_target_properties(CesiumAsync
    PROPERTIES
        TEST_SOURCES "${CESIUM_ASYNC_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_ASYNC_TEST_HEADERS}"
)

set_target_properties(CesiumAsync
    PROPERTIES
    PUBLIC_HEADER "${CESIUM_ASYNC_PUBLIC_HEADERS}"
)

target_sources(
    CesiumAsync
    PRIVATE
        ${CESIUM_ASYNC_SOURCES}
        ${CESIUM_ASYNC_HEADERS}
    PUBLIC
        ${CESIUM_ASYNC_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumAsync
    SYSTEM PUBLIC
        ${CESIUM_NATIVE_RAPIDJSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_link_libraries(CesiumAsync
    PUBLIC
        CesiumUtility
        GSL
        Async++
    PRIVATE
        sqlite3
)

# These libraries erroneously do NOT list their headers as `SYSTEM` headers
target_link_libraries_system(CesiumAsync PUBLIC
    spdlog
)

install(TARGETS CesiumAsync
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumAsync
)

install(
    DIRECTORY include/CesiumAsync/Impl
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumAsync
)
