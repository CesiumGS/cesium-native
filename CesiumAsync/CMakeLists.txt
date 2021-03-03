add_library(CesiumAsync "")

configure_cesium_library(CesiumAsync)

cesium_glob_files(CESIUM_ASYNC_SOURCES src/*.cpp)
cesium_glob_files(CESIUM_ASYNC_HEADERS src/*.h src/*.hpp)
cesium_glob_files(CESIUM_ASYNC_PUBLIC_HEADERS include/CesiumAsync/*.h)
cesium_glob_files(CESIUM_ASYNC_TEST_SOURCES test/*.cpp)
cesium_glob_files(CESIUM_ASYNC_TEST_HEADERS test/*.h)

set_target_properties(CesiumAsync 
    PROPERTIES 
        TEST_SOURCES "${CESIUM_ASYNC_TEST_SOURCES}"
        TEST_HEADERS "${CESIUM_ASYNC_TEST_HEADERS}"
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
        # SQLITE3 does not attach its include directory to
        # its target
        ${CESIUM_NATIVE_SQLITE3_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_link_libraries(CesiumAsync
    PUBLIC 
        GSL
        Async++
    # PRIVATE
        sqlite3 
)

# These libraries erroneously do NOT list their headers as `SYSTEM` headers
target_link_libraries_system(CesiumAsync PUBLIC
    spdlog
)

install(TARGETS CesiumAsync)
install(FILES ${CESIUM_ASYNC_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CesiumAsync)
