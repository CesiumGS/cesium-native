add_library(CesiumAsync "")

configure_cesium_library(CesiumAsync)

set(CESIUM_ASYNC_SOURCE
    src/AsyncSystem.cpp
)

set(CESIUM_ASYNC_PUBLIC_HEADERS
    include/CesiumAsync/AsyncSystem.h
    include/CesiumAsync/IAssetAccessor.h
    include/CesiumAsync/IAssetRequest.h
    include/CesiumAsync/IAssetResponse.h
    include/CesiumAsync/ITaskProcessor.h
    include/CesiumAsync/Library.h
)

target_sources(
    CesiumAsync
    PRIVATE
        ${CESIUM_ASYNC_SOURCE}
    PUBLIC
        ${CESIUM_ASYNC_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumAsync
    SYSTEM PUBLIC
    PUBLIC
        include
)

target_link_libraries(CesiumAsync PUBLIC Async++)
