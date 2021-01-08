add_library(CesiumAsync "")

configure_cesium_library(CesiumAsync)

file(GLOB SOURCES include/CesiumAsync/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumAsync/*.h)

target_sources(
    CesiumAsync
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumAsync
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/sqlite3
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/GSL/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/rapidjson/include

    PUBLIC
        include

        # Must set ${targetName}_PRIVATE_SRC to 1 to add src directory to the public include directory. 
        # This should only be set by the target itself and test target, and used for testing purpose only
        "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:CesiumAsync_PRIVATE_SRC>>:${CMAKE_CURRENT_SOURCE_DIR}/src>>"
)

target_link_libraries(CesiumAsync PUBLIC Async++ sqlite3)
