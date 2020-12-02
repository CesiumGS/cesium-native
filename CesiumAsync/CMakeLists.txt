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
    PUBLIC
        include
    PRIVATE
        src
)

target_link_libraries(CesiumAsync PUBLIC Async++)
