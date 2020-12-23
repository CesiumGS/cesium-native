add_library(CesiumGeospatial "")

configure_cesium_library(CesiumGeospatial)

file(GLOB SOURCES include/CesiumGeospatial/*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumGeospatial/*.h)

target_sources(
    CesiumGeospatial
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumGeospatial
    PUBLIC
        include

        # Must set ${targetName}_PRIVATE_SRC to 1 to add src directory to the public include directory. 
        # This should only be set by the target itself and test target, and used for testing purpose only
        "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:CesiumGeospatial_PRIVATE_SRC>>:${CMAKE_CURRENT_SOURCE_DIR}/src>>"
)

target_link_libraries(CesiumGeospatial PUBLIC CesiumGeometry CesiumUtility)
