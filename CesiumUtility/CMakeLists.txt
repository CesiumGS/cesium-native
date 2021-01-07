add_library(CesiumUtility "")

configure_cesium_library(CesiumUtility)

file(GLOB SOURCES include/CesiumUtility*.h src/*.cpp src/*.h)
file(GLOB PUBLIC_INCLUDES include/CesiumUtility/*.h)

target_sources(
    CesiumUtility
    PRIVATE
        ${SOURCES}
    PUBLIC
        ${PUBLIC_INCLUDES}
)

target_include_directories(
    CesiumUtility
    SYSTEM PUBLIC
        # This line shouldn't be unnecessary, because the glm include dirs will be set via the
        # target_link_libraries line below. But there is currently at least one warning (which gets turned
        # into an error) in a glm header, and there's no way to make the glm include directory a _system_
        # include directory via target_link_libraries (which would ignore the warning). So we add it as a
        # system include dir here.
        ${CMAKE_CURRENT_SOURCE_DIR}/../extern/glm
    PUBLIC
        include

        # Must set ${targetName}_PRIVATE_SRC to 1 to add src directory to the public include directory. 
        # This should only be set by the target itself and test target, and used for testing purpose only
        "$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:CesiumUtility_PRIVATE_SRC>>:${CMAKE_CURRENT_SOURCE_DIR}/src>>"
)

target_link_libraries(CesiumUtility PUBLIC GSL glm)
