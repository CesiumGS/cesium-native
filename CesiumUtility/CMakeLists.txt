add_library(CesiumUtility "")

configure_cesium_library(CesiumUtility)

set(CESIUM_UTILITY_SOURCE
    src/Library.cpp
)

set(CESIUM_UTILITY_TEST_SOURCE
    test/TestDoublyLinkedList.cpp
    test/TestMath.cpp
)

list(TRANSFORM CESIUM_UTILITY_TEST_SOURCE PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(CESIUM_UTILITY_TEST_SOURCE "${CESIUM_UTILITY_TEST_SOURCE}" PARENT_SCOPE)

set(CESIUM_UTILITY_PUBLIC_HEADERS
    include/CesiumUtility/DoublyLinkedList.h
    include/CesiumUtility/IntrusivePointer.h
    include/CesiumUtility/joinToString.h
    include/CesiumUtility/Library.h
    include/CesiumUtility/Math.h
)

set(CESIUM_UTILITY_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include" PARENT_SCOPE)

target_sources(
    CesiumUtility
    PRIVATE
        ${CESIUM_UTILITY_SOURCE}
    PUBLIC
        ${CESIUM_UTILITY_PUBLIC_HEADERS}
)

target_include_directories(
    CesiumUtility
    SYSTEM PUBLIC
        # This line shouldn't be unnecessary, because the glm include dirs will be set via the
        # target_link_libraries line below. But there is currently at least one warning (which gets turned
        # into an error) in a glm header, and there's no way to make the glm include directory a _system_
        # include directory via target_link_libraries (which would ignore the warning). So we add it as a
        # system include dir here.
        ${CESIUM_NATIVE_GLM_INCLUDE_DIR}
    PUBLIC
        include
)

target_link_libraries(CesiumUtility PUBLIC GSL glm)
