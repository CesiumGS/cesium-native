add_library(CesiumGeometry "")

configure_cesium_library(CesiumGeometry)

file(GLOB SOURCES include/CesiumGeometry/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGeometry/*.h)

target_sources(
    CesiumGeometry
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGeometry
    PUBLIC
        include

        # Must set ${targetName}_PRIVATE_SRC to 1 to add src directory to the public include directory. 
        # This should only be set by the target itself and test target, and used for testing purpose only
        "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:CesiumGeometry_PRIVATE_SRC>>:${CMAKE_CURRENT_SOURCE_DIR}/src>>"
)

target_link_libraries(CesiumGeometry PUBLIC CesiumUtility)
